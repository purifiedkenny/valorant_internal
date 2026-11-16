#pragma once

#include "model.hpp"
#include "../memory_map.hpp"
#include "../../driver/driver_context.hpp"
#include "../../utils/logger.hpp"
#include "fname_cache.hpp"
#include "fname_cache_context.hpp"
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>
#include <format>
#include <optional>
#include <cstring>

namespace maxi::game::model {

    class FNameCache;


    class UClass;
    
    
    
    
    
    class UObject {
    public:
        UObject() : m_base_address(0) {}


        explicit UObject(uintptr_t base_address)
            : m_base_address(base_address) {}


        uintptr_t get_base_address() const { return m_base_address; }
        void set_base_address(uintptr_t address) { m_base_address = address; }
        bool is_valid() const { return m_base_address != 0; }


        template<typename T>
        T read(uintptr_t offset = 0) const {
            if (!this->is_valid()) return T{};
            uintptr_t target = m_base_address + offset;


            T val{};
            if (maxi::driver::g_driver) {
                val = maxi::driver::g_driver->read<T>(target);
            }
            return val;
        }

        template<typename T>
        std::optional<T> TryRead(uintptr_t offset = 0) const {
            if (!this->is_valid()) return std::nullopt;
            uintptr_t target = m_base_address + offset;
            if (!maxi::driver::g_driver) return std::nullopt;
            return maxi::driver::g_driver->try_read<T>(target);
        }

		template<typename T>
        void write(uintptr_t offset, const T& value) const {
            if (!this->is_valid()) return;
            maxi::driver::g_driver->write<T>(m_base_address + offset, value);
		}

        uintptr_t get_vtable() const {
            return read<uintptr_t>(memory_map::UObjectVtable);
        }

        uint32_t get_flags() const {
            return read<uint32_t>(memory_map::UObjectFlags);
        }

        int32_t get_index() const {
            return read<int32_t>(memory_map::UObjectIndex);
        }

        UObject get_class() const {
            return read<UObject>(memory_map::UObjectClass);
        }

        UObject get_super() const {
            return read<UObject>(memory_map::UObjectSuper);
		}

        UObject get_outer() const {
            return read<UObject>(memory_map::UObjectOuter);
        }

        std::int32_t get_name() const {
            return read<std::int32_t>(memory_map::UObjectName);
        }

        bool is_a(const std::string& class_name) {
            if (!this) return false;

            auto current_class = get_class();
            if (!current_class.is_valid()) return false;

            UObject current_struct = current_class;
            while (current_struct.is_valid()) {
                std::string current_class_name = current_struct.get_name_string();
                if (current_class_name == class_name) {
                    return true;
                }

                current_struct = current_struct.get_super();
            }
            return false;
        }

        uintptr_t RootComponent() const {
            if (!is_valid()) return 0;
            return read<uintptr_t>(memory_map::RootComponent);
        }

        FVector RelativeLocation() const {
            if (!is_valid()) return {};
            auto root_comp = RootComponent();
            if (!root_comp) return {};
            return maxi::driver::g_driver->read<FVector>(root_comp + memory_map::RelativeLocation);
        }

        std::string get_name_string() const {
            return resolve_fname_key(get_name());
        }


        static std::string resolve_fname_key(std::int32_t key) {
            if(key == 0)
				return "None";

            if (auto cached = get_global_fname_cache().try_get(key))
            {
                return *cached;
            }


            static std::atomic<std::uint64_t> s_recovered_key{ 0 };
            static std::atomic<int> s_consec_fail{ 0 };
            static const int s_entry_stride = 4;

            const std::uint32_t chunkOffset = static_cast<std::uint32_t>(key >> 16);
            const std::uint16_t nameOffset = static_cast<std::uint16_t>(key & 0xFFFF);
            const std::uintptr_t chunkAddr = maxi::driver::g_driver->get_base_address() + memory_map::FNamePool + ((static_cast<unsigned long long>(chunkOffset) + 2ULL) * 8ULL);
            const std::uintptr_t namePoolChunk = maxi::driver::g_driver->read<std::uintptr_t>(chunkAddr);
            if (namePoolChunk == 0) return "None";

            std::uintptr_t nameKey = DecryptNameKey();
            if (!nameKey) {


                nameKey = static_cast<std::uintptr_t>(memory_map::FNameKeyStatic);
            }


            if (const auto rk = s_recovered_key.load()) {
                if (static_cast<std::uintptr_t>(rk) != nameKey) {
                    nameKey = static_cast<std::uintptr_t>(rk);
                    static bool s_inv = false;
                    if (!s_inv) {
                        s_inv = true;
                        get_global_fname_cache().invalidate_all();
                        LOG_WARNING(std::format("[FNAME] session key active: 0x{:08X} (cache cleared)", (unsigned)rk));
                    }
                }
            }

            const std::uintptr_t entryOffset = namePoolChunk + (static_cast<unsigned long long>(s_entry_stride) * nameOffset);
            FNameEntry nameEntry{};
            {
                uint8_t hdr[6] = {};
                if (!maxi::driver::g_driver->read_memory(entryOffset, hdr, sizeof(hdr))) {
                    get_global_fname_cache().put(key, "None");
                    return "None";
                }
                std::memcpy(&nameEntry.ComparisonId, hdr, 4);
                std::memcpy(&nameEntry.Header, hdr + 4, 2);
            }
            const std::uint16_t len = nameEntry.Header.Len;
            if (len == 0 || len > 1023) {
                get_global_fname_cache().put(key, "None");
                return "None";
            }
            if (nameEntry.IsWide()) {
                get_global_fname_cache().put(key, "None");
                return "None";
            }
            {
                static_assert(sizeof(nameEntry.AnsiName) >= 1024, "AnsiName buffer");
                if (!maxi::driver::g_driver->read_memory(entryOffset + 6, nameEntry.AnsiName, len)) {
                    get_global_fname_cache().put(key, "None");
                    return "None";
                }
            }

            std::string name(nameEntry.AnsiName, len);
            for (std::uint16_t i = 0; i < len; i++) {
                std::uint8_t b = i & 3;
                name[i] ^= static_cast<char>(len ^ *(((std::uint8_t*)&nameKey) + b));
            }


            auto is_printable_name = [](const std::string& s) {
                if (s.empty() || s.size() > 200) return false;
                for (unsigned char c : s) {
                    if (c >= 0x80) return false;
                    if (!(std::isalnum(c) || c == '_' || c == '.' || c == '/' ||
                          c == '-' || c == ':' || c == ' ' || c == '\''))
                        return false;
                }
                return true;
            };
            if (!is_printable_name(name))
            {

                std::uint64_t rk = s_recovered_key.load();
                if (rk == 0) {
                    const std::uintptr_t chunk0Addr = maxi::driver::g_driver->get_base_address() + memory_map::FNamePool + (2ULL * 8ULL);
                    auto chunk0_opt = maxi::driver::g_driver->try_read<std::uintptr_t>(chunk0Addr);
                    {
                        static bool s_diag_done = false;
                        if (!s_diag_done) {
                            s_diag_done = true;
                            const std::uintptr_t c0 = chunk0_opt ? *chunk0_opt : 0;
                            LOG_WARNING(std::format("[FNAME DIAG] base=0x{:X} pool=0x{:X} chunk0addr=0x{:X} chunk0=0x{:X} key=0x{:X} chunkOff={} nameOff={}",
                                maxi::driver::g_driver->get_base_address(), (unsigned long long)memory_map::FNamePool,
                                chunk0Addr, c0, (unsigned int)key,
                                (unsigned int)chunkOffset, (unsigned int)nameOffset));
                            if (c0) {
                                uint8_t raw[128] = {};
                                if (maxi::driver::g_driver->read_memory(c0, raw, sizeof(raw))) {
                                    std::string hx;
                                    hx.reserve(256);
                                    char tb[4];
                                    for (int bi = 0; bi < 128; ++bi) {
                                        snprintf(tb, sizeof(tb), "%02X", raw[bi]);
                                        hx += tb;
                                    }
                                    LOG_WARNING(std::format("[FNAME DIAG] chunk0[0..128]={}", hx));
                                } else {
                                    LOG_WARNING("[FNAME DIAG] chunk0 read failed (read_memory fail)");
                                }
                            }
                        }
                    }
                    if (chunk0_opt && *chunk0_opt) {
                        const std::uintptr_t chunk0 = *chunk0_opt;

                        std::uint32_t cand_keys[32] = {};
                        int cand_slots[32] = {};
                        std::uint32_t cand_cmpid[32] = {};
                        int cand_count = 0;
                        for (int slot = 0; slot < 16; ++slot) {
                            const std::uintptr_t cand = chunk0 + static_cast<std::uintptr_t>(slot) * 4ULL;
                            uint8_t hdr[6] = {};
                            if (!maxi::driver::g_driver->read_memory(cand, hdr, sizeof(hdr)))
                                continue;

                            const std::uint16_t rawHdr = static_cast<std::uint16_t>(hdr[4] | (hdr[5] << 8));
                            if (rawHdr & 0x1)
                                continue;
                            const std::uint16_t candLen = static_cast<std::uint16_t>(rawHdr >> 1);
                            if (candLen != 4)
                                continue;
                            uint8_t cipher[4] = {};
                            if (!maxi::driver::g_driver->read_memory(cand + 6, cipher, sizeof(cipher)))
                                continue;
                            const char* pt = "None";
                            std::uint32_t recovered = 0;
                            bool valid = true;
                            for (int i = 0; i < 4; ++i) {
                                std::uint8_t kk = static_cast<std::uint8_t>(cipher[i] ^ candLen ^ static_cast<std::uint8_t>(pt[i]));
                                ((std::uint8_t*)&recovered)[i] = kk;
                                if (kk == 0) { valid = false; break; }
                            }
                            if (valid && cand_count < 32) {
                                cand_keys[cand_count] = recovered;
                                cand_slots[cand_count] = slot;
                                cand_cmpid[cand_count] = static_cast<std::uint32_t>(hdr[0] | (hdr[1] << 8) | (hdr[2] << 16) | (hdr[3] << 24));
                                cand_count++;
                            }
                        }


                        for (int a = 0; a < cand_count && rk == 0; ++a) {
                            if (cand_slots[a] == 0 && cand_cmpid[a] == 0) {
                                s_recovered_key.store(cand_keys[a]);
                                LOG_WARNING(std::format("[FNAME] session key derived: 0x{:08X} (slot0/ComparisonId 0)", cand_keys[a]));
                                rk = cand_keys[a];
                            }
                        }
                        static bool s_no_consensus_logged = false;
                        for (int a = 0; a < cand_count && rk == 0; ++a) {
                            int votes = 0;
                            for (int b = 0; b < cand_count; ++b)
                                if (cand_keys[b] == cand_keys[a]) votes++;
                            if (votes >= 2) {
                                s_recovered_key.store(cand_keys[a]);
                                LOG_WARNING(std::format("[FNAME] session key derived: 0x{:08X} (votes {}/{})", cand_keys[a], votes, cand_count));
                                rk = cand_keys[a];
                            }
                        }
                        if (rk == 0 && cand_count > 0 && !s_no_consensus_logged) {
                            s_no_consensus_logged = true;
                            LOG_WARNING(std::format("[FNAME] no consensus ({} candidates, all different) - manual resolution required with DIAG lines", cand_count));
                        }
                    }
                }

                if (rk) {
                    nameKey = static_cast<std::uintptr_t>(rk);
                    for (std::uint16_t i = 0; i < len; i++) {
                        std::uint8_t b = i & 3;
                        name[i] = static_cast<char>(static_cast<std::uint8_t>(nameEntry.AnsiName[i]) ^ (len ^ *(((std::uint8_t*)&nameKey) + b)));
                    }
                    if (is_printable_name(name)) {
                        s_consec_fail.store(0);
                        get_global_fname_cache().put(key, name);
                        return name;
                    }
                    const int fails = s_consec_fail.fetch_add(1) + 1;
                    if (fails >= 8) {
                        s_consec_fail.store(0);
                        s_recovered_key.store(0);
                        LOG_WARNING(std::format("[FNAME] {} consecutive resolution failures - key reset, will re-derive", fails));
                    }
                }
                get_global_fname_cache().put(key, "None");
                return "None";
            }

			if (name.empty()) {
                get_global_fname_cache().put(key, "None");
                return "None";
            }

            if (!is_printable_name(name))
            {
                get_global_fname_cache().put(key, "None");
                return "None";
            }

            get_global_fname_cache().put(key, name);
            return name;
        }

        __forceinline static __int64 decrypt_xor_keys(const uint32_t StateKey, const uintptr_t* State)
        {
            uint64_t mixed = memory_map::derive::UObjectMixKey * static_cast<uint64_t>(StateKey ^ ((StateKey ^ (StateKey >> 15)) >> 12) ^ (StateKey << 25));
            uint32_t index = mixed % memory_map::derive::UObjectStateSlots;
            uint32_t high = static_cast<uint32_t>(mixed >> 32);
            uint64_t v = State[index];

            uint64_t sum = static_cast<uint64_t>(index) + high;
            unsigned int mod = static_cast<unsigned int>(sum % 63);
            unsigned int div = static_cast<unsigned int>(sum / 63);
            unsigned int low = static_cast<unsigned int>(sum % 256);
            unsigned int shiftR = mod + 1;
            unsigned int shiftL = 63 * div - low + 63;

            switch (index) {
            case 0:
                v = ~(v - (high - 1));
                break;
            case 1:
                v ^= static_cast<uint32_t>(index + high) ^ static_cast<uint32_t>(high + 2 * index);
                break;
            case 2: {
                uint64_t v9 = ((v >> 1) ^ ((v >> 1) ^ (v << 1)) & 0xAAAAAAAAAAAAAAAAULL) >> 2;
                uint64_t v10 = v9 ^ (v9 ^ (4ULL * ((v >> 1) ^ ((v >> 1) ^ (v << 1)) & 0xAAAAAAAAAAAAAAAAULL))) & 0xCCCCCCCCCCCCCCCCULL;
                uint64_t v11 = v10 >> 4;
                uint64_t temp12 = ((v11 ^ (v11 ^ (v10 << 4)) & 0xF0F0F0F0F0F0F0F0ULL) >> 8) ^
                    (((v11 ^ (v11 ^ (v10 << 4)) & 0xF0F0F0F0F0F0F0F0ULL) >> 8) ^
                        ((v11 ^ (v11 ^ (v10 << 4)) & 0xF0F0F0F0F0F0F0F0ULL) << 8)) & 0xFF00FF00FF00FF00ULL;
                uint64_t swapped = (temp12 << 32) | (temp12 >> 32);
                v = __ROR8__(swapped, shiftR);
                break;
            }
            case 3: {
                uint64_t v29 = ((v >> 1) ^ ((v >> 1) ^ (v << 1)) & 0xAAAAAAAAAAAAAAAAULL) >> 2;
                uint64_t v30 = v29 ^ (v29 ^ (4ULL * ((v >> 1) ^ ((v >> 1) ^ (v << 1)) & 0xAAAAAAAAAAAAAAAAULL))) & 0xCCCCCCCCCCCCCCCCULL;
                uint64_t v31 = v30 >> 4;
                uint64_t temp32 = ((v31 ^ (v31 ^ (v30 << 4)) & 0xF0F0F0F0F0F0F0F0ULL) >> 8) ^
                    (((v31 ^ (v31 ^ (v30 << 4)) & 0xF0F0F0F0F0F0F0F0ULL) >> 8) ^
                        ((v31 ^ (v31 ^ (v30 << 4)) & 0xF0F0F0F0F0F0F0F0ULL) << 8)) & 0xFF00FF00FF00FF00ULL;
                uint64_t swapped = (temp32 << 32) | (temp32 >> 32);
                v = __ROL8__(swapped, shiftR);
                break;
            }
            case 4:
                v += static_cast<uint64_t>(high + 2 * index) + static_cast<uint64_t>(index + high);
                break;
            case 5:
                v = (~v << shiftL) | (~v >> shiftR);
                break;
            case 6: {
                uint64_t v13 = ((v >> 1) ^ ((v >> 1) ^ (v << 1)) & 0xAAAAAAAAAAAAAAAAULL) >> 2;
                uint64_t v14 = v13 ^ (v13 ^ (4ULL * ((v >> 1) ^ ((v >> 1) ^ (v << 1)) & 0xAAAAAAAAAAAAAAAAULL))) & 0xCCCCCCCCCCCCCCCCULL;
                uint64_t v15 = (v14 >> 4) ^ ((v14 >> 4) ^ (v14 << 4)) & 0xF0F0F0F0F0F0F0F0ULL;
                uint64_t temp = (v15 >> 8) ^ ((v15 >> 8) ^ (v15 << 8)) & 0xFF00FF00FF00FF00ULL;
                uint64_t swapped = (temp << 32) | (temp >> 32);
                v = swapped - static_cast<uint32_t>(index + high);
                break;
            }
            }
            return v ^ StateKey;
        }

        static uintptr_t DecryptNameKey()
        {
            uintptr_t namepool_state_offset = memory_map::FNameState;
            uintptr_t namepool_key_offset = namepool_state_offset + memory_map::StateKey;

            uint64_t namepoolkey = maxi::driver::g_driver->read<uint64_t>(maxi::driver::g_driver->get_base_address() + namepool_key_offset);
            if (!namepoolkey)
                return 0;
#pragma pack(push, 1)
            struct State {
                uintptr_t keys[memory_map::derive::UObjectStateSlots];
            };
#pragma pack(pop)
            static_assert(sizeof(State) == sizeof(uintptr_t) * memory_map::derive::UObjectStateSlots,
                          "State slot count must match memory_map::derive::UObjectStateSlots");
            State xor_state = maxi::driver::g_driver->read<State>(maxi::driver::g_driver->get_base_address() + namepool_state_offset);
            auto address = decrypt_xor_keys(static_cast<uint32_t>(namepoolkey), reinterpret_cast<uintptr_t*>(&xor_state));
            uintptr_t xors = maxi::driver::g_driver->read<uintptr_t>(address);
            return xors;
        }

    protected:
        uintptr_t m_base_address;
    };

}
