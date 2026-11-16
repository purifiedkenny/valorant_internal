#pragma once

#include "ue_object.hpp"
#include <optional>

namespace maxi::game::model {

    class APlayerState : public UObject {
    public:
        APlayerState() : UObject() {}
        explicit APlayerState(uintptr_t base_address) : UObject(base_address) {}

        uintptr_t TeamComponent() const {
            if (!is_valid()) return 0;
            return read<uintptr_t>(memory_map::TeamComponent);
		}

        std::optional<uintptr_t> TryTeamComponent() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::TeamComponent);
        }

        int32_t TeamId() const {
            if (!is_valid()) return -1;
			auto team_component = TeamComponent();
            if (team_component == 0) return 0;
			return maxi::driver::g_driver->read<int32_t>(team_component + memory_map::TeamId);
        }

        std::optional<int32_t> TryTeamId() const {
            if (!is_valid()) return std::nullopt;
            auto tc_opt = TryTeamComponent();
            if (!tc_opt || !*tc_opt) return std::nullopt;
            return maxi::driver::g_driver->try_read<int32_t>(*tc_opt + memory_map::TeamId);
        }


        [[deprecated("Use TryTeamComponent()")]]
        uintptr_t TeamComponent_compat() const { return TeamComponent(); }

        [[deprecated("Use TryTeamId() - distinguishes 0 (no team) from read failure")]]
        int32_t TeamId_compat() const { return TeamId(); }
    };
}