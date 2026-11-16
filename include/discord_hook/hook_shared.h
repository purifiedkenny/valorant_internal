#pragma once








#include <cstdint>

namespace discord_hook {


#pragma pack(push, 1)


struct ActorData {
    float head_x, head_y;
    float neck_x, neck_y;
    float chest_x, chest_y;
    float pelvis_x, pelvis_y;
    float l_shoulder_x, l_shoulder_y;
    float l_elbow_x, l_elbow_y;
    float l_wrist_x, l_wrist_y;
    float r_shoulder_x, r_shoulder_y;
    float r_elbow_x, r_elbow_y;
    float r_wrist_x, r_wrist_y;
    float l_hip_x, l_hip_y;
    float l_knee_x, l_knee_y;
    float l_ankle_x, l_ankle_y;
    float r_hip_x, r_hip_y;
    float r_knee_x, r_knee_y;
    float r_ankle_x, r_ankle_y;
    float screen_w, screen_h;
    uint32_t team_id;
    bool is_visible;
    bool is_valid;
    char agent_name[32];
};

struct SharedFrameData {
    uint64_t frame_id;
    uint32_t actor_count;
    ActorData actors[64];
    float menu_scale;
    bool show_menu;
    bool show_boxes;
    bool show_skeleton;
    bool show_names;
    bool show_health;
    uint32_t screen_width;
    uint32_t screen_height;
};
#pragma pack(pop)


constexpr const wchar_t* SHARED_MEMORY_NAME = L"Global\\MAXI_DISCORD_HOOK_SHARED";


constexpr const wchar_t* SYNC_MUTEX_NAME = L"Global\\MAXI_DISCORD_HOOK_MUTEX";

}
