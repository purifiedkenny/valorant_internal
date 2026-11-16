#pragma once

#include "ue_object.hpp"
#include "aactor.hpp"
#include "aplayer_camera_manager.hpp"
#include <optional>

namespace maxi::game::model {

    class APlayerController : public UObject {
    public:
        APlayerController() : UObject() {}
        explicit APlayerController(uintptr_t base_address) : UObject(base_address) {}

        AActor get_acknowledged_pawn() const {
            if (!is_valid()) return {};
            return read<AActor>(memory_map::AcknowledgedPawn);
		}

        std::optional<AActor> TryGetAcknowledgedPawn() const {
            if (!is_valid()) return std::nullopt;
            auto pawn_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::AcknowledgedPawn);
            if (!pawn_opt || !*pawn_opt) return std::nullopt;
            return AActor(*pawn_opt);
        }

        APlayerCameraManager get_player_camera_manager() const {
            if (!is_valid()) return {};
            return read<APlayerCameraManager>(memory_map::PlayerCameraManager);
		}

        std::optional<APlayerCameraManager> TryGetPlayerCameraManager() const {
            if (!is_valid()) return std::nullopt;
            auto pcm_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::PlayerCameraManager);
            if (!pcm_opt || !*pcm_opt) return std::nullopt;
            return APlayerCameraManager(*pcm_opt);
        }

        FRotator ControlRotation() const {
            if (!is_valid()) return {};
            return read<FRotator>(memory_map::ControlRotation);
        }

        std::optional<FRotator> TryControlRotation() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<FRotator>(m_base_address + memory_map::ControlRotation);
        }

        void SetControlRotation(const FRotator& rotation) {
            if (!is_valid()) return;
            write<FRotator>(memory_map::ControlRotation, rotation);
		}


        [[deprecated("Use TryGetAcknowledgedPawn()")]]
        AActor get_acknowledged_pawn_compat() const { return get_acknowledged_pawn(); }

        [[deprecated("Use TryGetPlayerCameraManager()")]]
        APlayerCameraManager get_player_camera_manager_compat() const { return get_player_camera_manager(); }

        [[deprecated("Use TryControlRotation()")]]
        FRotator ControlRotation_compat() const { return ControlRotation(); }
    };

}


