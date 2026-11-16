#include <include/core/actors_data_provider.hpp>
#include <include/game/engine/core_engine.hpp>
#include <include/render/render.hpp>

namespace maxi::core {

static ActorSnapshot g_snapshot;
static std::mutex g_snapshot_mutex;

const ActorSnapshot& GetActorsSnapshot() {
    return g_snapshot;
}

void UpdateActorsSnapshot(const ActorSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(g_snapshot_mutex);
    g_snapshot = snapshot;
}

}