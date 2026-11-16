#pragma once

#include "ue_object.hpp"
#include "aplayer_controller.hpp"
#include <vector>
#include <optional>

namespace maxi::game::model {

    
    
    
    
    class ULocalPlayer : public UObject {
    public:
        ULocalPlayer() : UObject() {}


        explicit ULocalPlayer(uintptr_t base_address)
            : UObject(base_address) {
        }

        APlayerController get_player_controller() const {
            if (!is_valid()) return {};
            return read<APlayerController>(memory_map::PlayerController);
        }

        std::optional<APlayerController> TryGetPlayerController() const {
            if (!is_valid()) return std::nullopt;
            auto pc_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::PlayerController);
            if (!pc_opt || !*pc_opt) return std::nullopt;
            return APlayerController(*pc_opt);
        }


        [[deprecated("Use TryGetPlayerController()")]]
        APlayerController get_player_controller_compat() const { return get_player_controller(); }

        void print_debug() const;
    };
}