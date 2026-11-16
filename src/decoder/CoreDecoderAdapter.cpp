



#include "CoreDecoderAdapter.hpp"

#include <cstring>
#include <cstdio>
#include <string>

#include "../../include/driver/xor_crypt.hpp"
#include "../../include/utils/logger.hpp"

namespace maxi::core::decoder {

    namespace {

        CoreRuntimeConfig g_runtime{};
        CoreDecoderMetadata g_metadata{};
        bool g_initialized = false;


        void derive_runtime_config(const CoreDecoderMetadata& md,
                                   CoreRuntimeConfig& out) {
            out.version = md.version;
            out.key_len = static_cast<uint32_t>(CORE_KEY_CAP);
            out.active  = true;

            uint32_t h = md.build_stamp ^ 0x9E3779B9;
            for (uint32_t i = 0; i < out.key_len; ++i) {
                const uint8_t pb = (md.poly_block && md.poly_block_size > 0)
                    ? md.poly_block[i % md.poly_block_size]
                    : static_cast<uint8_t>(i);
                h ^= static_cast<uint32_t>(pb) + 0x9E3779B9u +
                     (h << 6) + (h >> 2);
                h += 0x85EBCA6B + i * 0xC2B2AE35;
                out.key[i] = static_cast<uint8_t>((h >> 13) & 0xFF);
            }
        }

    }

    const char* validate_str(CoreValidateResult r) {
        switch (r) {
            case CoreValidateResult::OK:               return "OK";
            case CoreValidateResult::NOT_INITIALIZED:  return "NOT_INITIALIZED";
            case CoreValidateResult::VERSION_MISMATCH: return "VERSION_MISMATCH";
            case CoreValidateResult::KEYLEN_INVALID:   return "KEYLEN_INVALID";
            case CoreValidateResult::CHECKSUM_MISMATCH:return "CHECKSUM_MISMATCH";
        }
        return "UNKNOWN";
    }

    uint32_t compute_checksum(const uint8_t* data, size_t size) {
        uint32_t h = 5381;
        for (size_t i = 0; i < size; ++i) {
            h = ((h << 5) + h) + data[i];
        }
        return h;
    }

    CoreValidateResult validate_decoder_config(const CoreRuntimeConfig& cfg,
                                               uint32_t metadata_version) {
        if (!cfg.active) {
            std::printf("  [VALIDATOR] FAIL: runtime config not initialized\n");
            return CoreValidateResult::NOT_INITIALIZED;
        }
        if (cfg.version != metadata_version) {
            std::printf("  [VALIDATOR] FAIL: config version %u != metadata version %u\n",
                        cfg.version, metadata_version);
            return CoreValidateResult::VERSION_MISMATCH;
        }
        if (cfg.key_len == 0 || cfg.key_len > CORE_KEY_CAP) {
            std::printf("  [VALIDATOR] FAIL: invalid key length %u\n", cfg.key_len);
            return CoreValidateResult::KEYLEN_INVALID;
        }
        return CoreValidateResult::OK;
    }

    bool test_decoder_roundtrip(const CoreRuntimeConfig& cfg) {
        if (!cfg.active || cfg.key_len == 0) return false;

        constexpr size_t N = 64;
        uint8_t plain[N];
        uint8_t encoded[N];
        uint8_t decoded[N];

        for (size_t i = 0; i < N; ++i) {
            plain[i] = static_cast<uint8_t>(0x41 + (i % 26));
        }


        for (size_t i = 0; i < N; ++i) {
            encoded[i] = static_cast<uint8_t>(plain[i] ^ cfg.key[i % cfg.key_len]);
        }

        for (size_t i = 0; i < N; ++i) {
            decoded[i] = static_cast<uint8_t>(encoded[i] ^ cfg.key[i % cfg.key_len]);
        }

        return std::memcmp(plain, decoded, N) == 0;
    }

    bool initialize_with_metadata(const CoreDecoderMetadata& metadata) {
        if (metadata.version == 0 ||
            metadata.poly_block == nullptr ||
            metadata.poly_block_size == 0) {
            LOG_ERROR("CoreDecoderAdapter: invalid metadata (version/poly_block)");
            g_initialized = false;
            g_runtime.active = false;
            return false;
        }

        g_metadata = metadata;
        derive_runtime_config(metadata, g_runtime);

        auto vr = validate_decoder_config(g_runtime, metadata.version);
        if (vr != CoreValidateResult::OK) {
            LOG_ERROR(std::string("CoreDecoderAdapter: config validation failed: ") +
                      validate_str(vr));
            g_runtime.active = false;
            g_initialized = false;
            return false;
        }
        if (!test_decoder_roundtrip(g_runtime)) {
            LOG_ERROR("CoreDecoderAdapter: initial roundtrip test failed");
            g_runtime.active = false;
            g_initialized = false;
            return false;
        }

        g_runtime.active = true;
        g_initialized = true;

        char stamp_hex[16] = {};
        std::snprintf(stamp_hex, sizeof(stamp_hex), "0x%08X", metadata.build_stamp);
        LOG_INFO("CoreDecoderAdapter: initialized with metadata version=" +
                 std::to_string(metadata.version) + " build_stamp=" + stamp_hex);
        return true;
    }

    bool validate_core_decoders() {
        if (!g_initialized) {
            LOG_ERROR("CoreDecoderAdapter: validate called before initialize");
            return false;
        }

        int failures = 0;


        auto vr = validate_decoder_config(g_runtime, g_metadata.version);
        std::printf("[CV1] validate_decoder_config -> %s\n", validate_str(vr));
        if (vr != CoreValidateResult::OK) ++failures;


        const bool rt = test_decoder_roundtrip(g_runtime);
        std::printf("[CV2] test_decoder_roundtrip -> %s\n", rt ? "PASS" : "FAIL");
        if (!rt) ++failures;

        {
            constexpr size_t N = 32;
            uint8_t buf[N];
            for (size_t i = 0; i < N; ++i) {
                buf[i] = static_cast<uint8_t>(g_runtime.key[i % g_runtime.key_len]) ^
                         static_cast<uint8_t>(0x10 + i);
            }
            const uint32_t expect = compute_checksum(buf, N);
            uint8_t corrupted[N];
            std::memcpy(corrupted, buf, N);
            corrupted[0] ^= 0xFF;

            const bool ok_clean = (compute_checksum(buf, N) == expect);
            const bool ok_corrupt_detected =
                (compute_checksum(corrupted, N) != expect);

            std::printf("[CV3] checksum clean=%s corrupt_detected=%s -> %s\n",
                        ok_clean ? "true" : "false",
                        ok_corrupt_detected ? "true" : "false",
                        (ok_clean && ok_corrupt_detected) ? "PASS" : "FAIL");
            if (!(ok_clean && ok_corrupt_detected)) ++failures;
        }

        {
            constexpr size_t N = 48;
            uint8_t plain[N];
            uint8_t work[N];
            for (size_t i = 0; i < N; ++i) {
                plain[i] = static_cast<uint8_t>(0x61 + (i % 26));
                work[i]  = plain[i];
            }
            driver::crypt::full_encrypt(work, N);
            driver::crypt::full_decrypt(work, N);
            const bool ok = (std::memcmp(plain, work, N) == 0);
            std::printf("[CV4] static crypt roundtrip -> %s\n", ok ? "PASS" : "FAIL");
            if (!ok) ++failures;
        }


        {
            CoreRuntimeConfig rederived{};
            derive_runtime_config(g_metadata, rederived);
            const bool ok = (std::memcmp(rederived.key, g_runtime.key,
                                         CORE_KEY_CAP) == 0 &&
                             rederived.version == g_runtime.version);
            std::printf("[CV5] metadata binding consistency -> %s\n",
                        ok ? "PASS" : "FAIL");
            if (!ok) ++failures;
        }

        std::printf("[VALIDATE] core decoders: %s (%d failure)\n",
                    failures == 0 ? "PASS" : "FAIL", failures);

        if (failures != 0) {
            LOG_ERROR("CoreDecoderAdapter: validate_core_decoders failed with " +
                      std::to_string(failures) + " failure(s)");
            return false;
        }
        return true;
    }

    const CoreRuntimeConfig& get_runtime_config() {
        return g_runtime;
    }

    bool is_initialized() {
        return g_initialized;
    }

    void transform_with_runtime_key(uint8_t* data, size_t size) {
        if (!data || !g_initialized) return;
        const uint32_t klen = g_runtime.key_len;
        for (size_t i = 0; i < size; ++i) {
            data[i] ^= g_runtime.key[i % klen];
        }
    }

    void untransform_with_runtime_key(uint8_t* data, size_t size) {
        transform_with_runtime_key(data, size);
    }

}
