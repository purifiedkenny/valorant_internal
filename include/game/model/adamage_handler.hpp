#pragma once

#include "ue_object.hpp"
#include "aplayer_state.hpp"
#include <optional>

namespace maxi::game::model {
    
    
    
    
    class ADamageHandler : public UObject {
    public:
        ADamageHandler() : UObject() {}
		explicit ADamageHandler(uintptr_t base_address) : UObject(base_address) {}


        std::optional<float> TryGetHealth() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<float>(m_base_address + memory_map::Health);
		}

        std::optional<float> TryGetShield() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<float>(m_base_address + memory_map::Shield);
		}

        std::optional<float> TryGetMaxShield() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<float>(m_base_address + memory_map::MaxShield);
		}

        std::optional<int32_t> TryGetShieldType() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<int32_t>(m_base_address + memory_map::ShieldType);
		}


        [[deprecated("Use TryGetHealth() - distinguishes 0.0f (dead) from read failure")]]
        float GetHealth() const { return TryGetHealth().value_or(0.0f); }

        [[deprecated("Use TryGetShield() - distinguishes 0.0f (no shield) from read failure")]]
        float GetShield() const { return TryGetShield().value_or(0.0f); }

        [[deprecated("Use TryGetMaxShield() - distinguishes 0.0f (no shield) from read failure")]]
        float GetMaxShield() const { return TryGetMaxShield().value_or(0.0f); }

        [[deprecated("Use TryGetShieldType()")]]
        int32_t GetShieldType() const { return TryGetShieldType().value_or(0); }
	};
}