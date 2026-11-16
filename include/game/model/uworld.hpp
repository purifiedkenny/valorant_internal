#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>

#include "ue_object.hpp"
#include "ulevel.hpp"
#include "ugame_instance.hpp"

namespace maxi::game::model {

    
    
    
    
    class UWorld : public UObject {
    public:
        UWorld() : UObject() {}

        explicit UWorld(uintptr_t base_address)
            : UObject(base_address) {
        }

        uintptr_t get_uworld_address() const { return get_base_address(); }
        bool is_uworld_valid() const { return get_base_address() != 0; }

        UGameInstance get_game_instance() const {
            const auto gi_addr = read<uintptr_t>(memory_map::OwningGameInstance);
            return UGameInstance(gi_addr);
		}

        std::optional<UGameInstance> TryGetGameInstance() const {
            if (!is_valid()) return std::nullopt;
            auto gi_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::OwningGameInstance);
            if (!gi_opt || !*gi_opt) return std::nullopt;
            return UGameInstance(*gi_opt);
        }

        ULevel get_persistent_level() const {
            const uintptr_t upper = maxi::driver::g_driver
                ? maxi::driver::g_driver->get_base_address()
                : (uintptr_t)0x7FFFFFFF0000ull;
            const auto valid = [&](uintptr_t a) {
                return a && a > 0x10000 && a < upper && a != get_base_address();
            };

            const auto level_addr = read<uintptr_t>(memory_map::PersistentLevel);
            if (valid(level_addr)) {
                return ULevel(level_addr);
            }

            for (uintptr_t off : memory_map::PersistentLevelFallbacks) {
                const auto addr = read<uintptr_t>(off);
                if (valid(addr)) return ULevel(addr);
            }
            return ULevel(0);
        }

        std::optional<ULevel> TryGetPersistentLevel() const {
            if (!is_valid()) return std::nullopt;
            const uintptr_t upper = maxi::driver::g_driver
                ? maxi::driver::g_driver->get_base_address()
                : (uintptr_t)0x7FFFFFFF0000ull;
            const auto valid = [&](uintptr_t a) {
                return a && a > 0x10000 && a < upper && a != get_base_address();
            };

            auto level_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::PersistentLevel);
            if (level_opt && *level_opt && valid(*level_opt)) {
                return ULevel(*level_opt);
            }

            for (uintptr_t off : memory_map::PersistentLevelFallbacks) {
                auto addr_opt = maxi::driver::g_driver->try_read<uintptr_t>(off);
                if (addr_opt && *addr_opt && valid(*addr_opt)) {
                    return ULevel(*addr_opt);
                }
            }
            return std::nullopt;
        }


        [[deprecated("Use TryGetGameInstance()")]]
        UGameInstance get_game_instance_compat() const { return get_game_instance(); }

        [[deprecated("Use TryGetPersistentLevel()")]]
        ULevel get_persistent_level_compat() const { return get_persistent_level(); }
    };
}