#pragma once









#include <cstdint>
#include <cstddef>

#include "../../include/core/build_stamp.hpp"

namespace maxi::core::decoder {

    constexpr uint32_t CORE_DECODER_VERSION = 1;
    constexpr size_t   CORE_KEY_CAP         = 16;

    struct CoreDecoderMetadata {
        uint32_t       version;
        uint32_t       build_stamp;
        const uint8_t* poly_block;
        uint32_t       poly_block_size;
    };


    struct CoreRuntimeConfig {
        uint32_t version;
        uint8_t  key[CORE_KEY_CAP];
        uint32_t key_len;
        bool     active;
    };

    enum class CoreValidateResult {
        OK,
        NOT_INITIALIZED,
        VERSION_MISMATCH,
        KEYLEN_INVALID,
        CHECKSUM_MISMATCH,
    };

    const char* validate_str(CoreValidateResult r);


    CoreValidateResult validate_decoder_config(const CoreRuntimeConfig& cfg,
                                               uint32_t metadata_version);
    bool test_decoder_roundtrip(const CoreRuntimeConfig& cfg);

    inline CoreDecoderMetadata make_default_metadata() {
        return CoreDecoderMetadata{
            CORE_DECODER_VERSION,
            maxi::build::BUILD_STAMP,
            maxi::build::POLY_BLOCK,
            static_cast<uint32_t>(sizeof(maxi::build::POLY_BLOCK))
        };
    }

    bool initialize_with_metadata(const CoreDecoderMetadata& metadata);


    bool validate_core_decoders();

    const CoreRuntimeConfig& get_runtime_config();
    bool is_initialized();


    void transform_with_runtime_key(uint8_t* data, size_t size);
    void untransform_with_runtime_key(uint8_t* data, size_t size);

    uint32_t compute_checksum(const uint8_t* data, size_t size);

}
