#pragma once

#include "ue_object.hpp"
#include <optional>

namespace maxi::game::model {

    
    
    
    
    class ULevel : public UObject {
    public:
        ULevel() : UObject() {}


        explicit ULevel(uintptr_t base_address)
            : UObject(base_address) {}



        static bool IsHeapPtr(uintptr_t p) {
            if (!p || p < 0x10000) return false;
            auto base = maxi::driver::g_driver ? maxi::driver::g_driver->get_base_address() : 0;
            const uintptr_t upper = base ? base : (uintptr_t)0x7FFFFFFF0000ull;
            return p < upper;
        }

        static bool ValidActorArray(const TArray<AActor>& arr) {
            if (arr.Count <= 0 || arr.Count >= 5000) return false;
            const uintptr_t data = reinterpret_cast<uintptr_t>(arr.Data);
            if (!IsHeapPtr(data)) return false;
            if (!maxi::driver::g_driver) return true;
            const int32_t n = (arr.Count < 8) ? arr.Count : 8;
            for (int32_t i = 0; i < n; ++i) {
                auto e = maxi::driver::g_driver->try_read<uintptr_t>(data + (uintptr_t)i * sizeof(uintptr_t));
                if (e && IsHeapPtr(*e)) return true;
            }
            return false;
        }

        TArray<AActor> get_actors() const {
            if (!is_valid()) return TArray<AActor>();

            for (uintptr_t off : memory_map::ActorArrayCandidates) {
                TArray<AActor> res{};
                uintptr_t target_ptr = get_base_address() + off;

                bool success = false;
                __try {
                    res = *reinterpret_cast<const TArray<AActor>*>(target_ptr);
                    success = true;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    res = TArray<AActor>{};
                }

                if (success && ValidActorArray(res)) {
                    return res;
                }

                if (maxi::driver::g_driver) {
                    maxi::driver::g_driver->read_memory(target_ptr, &res, sizeof(TArray<AActor>));
                    if (ValidActorArray(res)) {
                        return res;
                    }
                }
            }
            return TArray<AActor>();
        }



        static bool TryProbeActorArray(uintptr_t target_ptr, TArray<AActor>& out_array) {
            bool success = false;
            __try {
                out_array = *reinterpret_cast<const TArray<AActor>*>(target_ptr);
                success = true;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
            return success;
        }

        std::optional<TArray<AActor>> TryGetActors() const {
            if (!is_valid()) return std::nullopt;

            for (uintptr_t off : memory_map::ActorArrayCandidates) {
                uintptr_t target_ptr = get_base_address() + off;

                TArray<AActor> res{};
                if (TryProbeActorArray(target_ptr, res) && ValidActorArray(res)) {
                    return std::optional<TArray<AActor>>(res);
                }

                if (maxi::driver::g_driver) {
                    TArray<AActor> probe{};
                    maxi::driver::g_driver->read_memory(target_ptr, &probe, sizeof(TArray<AActor>));
                    if (ValidActorArray(probe)) {
                        return std::optional<TArray<AActor>>(probe);
                    }
                }
            }
            return std::nullopt;
        }


        [[deprecated("Use TryGetActors()")]]
        TArray<AActor> get_actors_compat() const { return get_actors(); }
    };
}
