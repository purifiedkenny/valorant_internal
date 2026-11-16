#pragma once

#include <vector>
#include <mutex>
#include <game/engine/core_engine.hpp>

namespace maxi::core {


struct ActorSnapshot {
    struct Bone {
        float x, y, z;
    };
    struct Actor {
        std::array<Bone, 17> world_bones;
        float screen_x, screen_y;
        float health;
        int team_id;
        bool is_teammate;
        bool visible;
        std::string name;
    };
    std::vector<Actor> actors;
    float screen_width = 0;
    float screen_height = 0;
    uint64_t frame_id = 0;
    std::chrono::steady_clock::time_point timestamp;
};


const ActorSnapshot& GetActorsSnapshot();


void UpdateActorsSnapshot(const ActorSnapshot& snapshot);

}