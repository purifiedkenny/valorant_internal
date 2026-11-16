#pragma once

#include "ue_object.hpp"
#include <optional>

namespace maxi::game::model {

    class APlayerCameraManager : public UObject {
    public:
        APlayerCameraManager() : UObject() {}
        explicit APlayerCameraManager(uintptr_t base_address) : UObject(base_address) {}

        FMinimalViewInfo get_camera_view() const {
            if (!is_valid()) return {};

            uintptr_t pov_addr = get_base_address() + memory_map::CameraCache + memory_map::ViewTargetPOV;
            return maxi::driver::g_driver->read<FMinimalViewInfo>(pov_addr);
		}

        std::optional<FMinimalViewInfo> TryGetCameraView() const {
            if (!is_valid()) return std::nullopt;
            uintptr_t pov_addr = get_base_address() + memory_map::CameraCache + memory_map::ViewTargetPOV;
            return maxi::driver::g_driver->try_read<FMinimalViewInfo>(pov_addr);
        }


        [[deprecated("Use TryGetCameraView()")]]
        FMinimalViewInfo get_camera_view_compat() const { return get_camera_view(); }
    };

}