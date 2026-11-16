#include <include/common.hpp>
#include <include/game/engine/core_engine.hpp>
#include <include/render/settings/variables.h>
#include <include/game/memory_map.hpp>
#include <include/utils/logger.hpp>
#include <include/game/model/fname_cache_context.hpp>
#include <unordered_set>
#include <unordered_map>
#include <cstring>
#include <cstdint>
#include <limits>

namespace maxi::game::engine {
    using namespace maxi::utils;

    CoreEngine::CoreEngine() {
    }

    CoreEngine::~CoreEngine() {
        shutdown();
    }

    bool CoreEngine::initialize() {

        if (!set_target_process(maxi::driver::target_process_name_w())) {
            LOG_INFO("Target process not running yet, waiting for launch...");
        }
        m_kernel_protocol = std::make_unique<KernelProtocol>();
        return true;
    }

    void CoreEngine::shutdown() {
        if (m_running.load()) {
            stop();
        }


        if (m_world_thread && m_world_thread->joinable()) {
            m_world_thread->join();
        }
        if (m_camera_thread && m_camera_thread->joinable()) {
            m_camera_thread->join();
        }
        if (m_actors_thread && m_actors_thread->joinable()) {
            m_actors_thread->join();
        }


        m_kernel_protocol.reset();
    }

    void CoreEngine::start() {
        if (m_running.load()) {
            return;
        }

        if (!m_kernel_protocol) {
            return;
        }

        m_running.store(true);
        m_paused.store(false);


        m_world_thread = std::make_unique<std::thread>([this]() {

            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
            world_thread_loop();
        });


        m_camera_thread = std::make_unique<std::thread>([this]() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
            camera_thread_loop();
        });


        m_actors_thread = std::make_unique<std::thread>([this]() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
            actors_thread_loop();
        });


		m_skills_thread = std::make_unique<std::thread>([this]() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
			skills_thread_loop();
		});


        m_bones_thread = std::make_unique<std::thread>([this]() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
            bones_thread_loop();
        });
    }

    void CoreEngine::stop() {
        if (!m_running.load()) {
            return;
        }

        m_running.store(false);


        if (m_world_thread && m_world_thread->joinable()) {
            m_world_thread->join();
        }
        if (m_camera_thread && m_camera_thread->joinable()) {
            m_camera_thread->join();
        }
        if (m_actors_thread && m_actors_thread->joinable()) {
            m_actors_thread->join();
        }
        if (m_bones_thread && m_bones_thread->joinable()) {
            m_bones_thread->join();
        }


        if (m_skills_thread && m_skills_thread->joinable()) {
            m_skills_thread->join();
        }
    }

    void CoreEngine::pause() {
        m_paused.store(true);
    }

    void CoreEngine::resume() {
        m_paused.store(false);
    }

    bool CoreEngine::set_target_process(const std::wstring& process_name) {
        if (!maxi::driver::g_driver) {
            return false;
        }

        if (!maxi::driver::g_driver->attach_process(process_name)) {
            return false;
        }
        return true;
    }

    bool CoreEngine::set_target_process(uint32_t process_id) {
        if (!maxi::driver::g_driver) {
            return false;
        }

        if (!maxi::driver::g_driver->attach_process(process_id)) {
            return false;
        }
        return true;
    }

    WorldData CoreEngine::get_world_data() const {
        return m_world_data;
    }

    ActorsData CoreEngine::get_actors_data() const {
        if (m_actors_data_mutex.try_lock())
        {
			ActorsData copy = m_actors_data;
            m_actors_data_mutex.unlock();
			return copy;
        }
        return {};
    }

    SkillsData CoreEngine::get_skills_data() const {
        if (m_actors_data_mutex.try_lock())
        {
            SkillsData copy = m_skills_data;
            m_actors_data_mutex.unlock();
            return copy;
        }
        return {};
    }

    void CoreEngine::set_world_update_callback(std::function<void(const WorldData&)> callback) {
        m_world_callback = std::move(callback);
    }

    void CoreEngine::set_actors_update_callback(std::function<void(const ActorsData&)> callback) {
        m_actors_callback = std::move(callback);
    }


    uintptr_t CoreEngine::resolve_world_address() {
        static uintptr_t s_world_cache = 0;
        static uint32_t s_world_pid = 0;
        static ULONGLONG s_world_t = 0;

        const uint32_t pid = maxi::driver::g_driver ? maxi::driver::g_driver->get_process_id() : 0;
        const ULONGLONG now = GetTickCount64();
        if (s_world_cache && pid && pid == s_world_pid && (now - s_world_t) < 500)
            return s_world_cache;

        uintptr_t r = resolve_world_address_uncached();
        if (r) { s_world_cache = r; s_world_pid = pid; s_world_t = now; }
        else   { s_world_cache = 0; s_world_t = now; }
        return r;
    }

    uintptr_t CoreEngine::resolve_world_address_uncached() {
        if (!maxi::driver::g_driver) {
            LOG_ERROR("[DIAG] g_driver is null!");
            return 0;
        }

        auto base = maxi::driver::g_driver->get_base_address();

        auto level_present = [base](uintptr_t w) -> bool {


            static const uintptr_t offs[] = { 0x280, 0x398, 0x008,
                                              memory_map::PersistentLevel, 0x40, 0x48 };


            const uintptr_t upper = base ? base : (uintptr_t)0x7FFFFFFF0000ull;
            static bool dbg = false;
            if (!dbg) {
                for (uintptr_t o : offs) {
                    auto l = maxi::driver::g_driver->try_read<uintptr_t>(w + o).value_or(0);
                    LOG_INFO(std::format("[LP DIAG] w=0x{:X} off=0x{:X} -> 0x{:X} (dtb=0x{:X})",
                        w, o, l, maxi::driver::g_driver->get_dtb()));
                }
                dbg = true;
            }
            for (uintptr_t o : offs) {
                auto l = maxi::driver::g_driver->try_read<uintptr_t>(w + o).value_or(0);
                if (l && l > 0x10000 && l < upper && l != w) return true;
            }
            return false;
        };

        if (m_kernel_protocol) {

            auto pml4 = m_kernel_protocol->find_pml4_base();
            if (pml4.PML4Base) {
                for (auto off : memory_map::GWorldPml4Candidates) {
                    if (!off) continue;
                    auto w_ptr = maxi::driver::g_driver->try_read<uintptr_t>(pml4.PML4Base + off).value_or(0);
                    if (w_ptr && w_ptr > 0x10000 && level_present(w_ptr)) {
                        static bool logged_pml4 = false;
                        if (!logged_pml4) {
                            LOG_INFO(std::format("[DIAG SUCCESS] PML4 Resolved UWorld -> 0x{:X} (Offset: 0x{:X})", w_ptr, off));
                            logged_pml4 = true;
                        }
                        return w_ptr;
                    }
                }
            }
        }

        if (base) {


            static bool s_static_gworld_ok = false;
            if (!s_static_gworld_ok)
            for (uintptr_t state_off : memory_map::UWorldStateCandidates) {
                if (!state_off) continue;
                uint32_t key38 = maxi::driver::g_driver->try_read<uint32_t>(base + state_off + memory_map::StateKey).value_or(0);
                uint32_t key0  = maxi::driver::g_driver->try_read<uint32_t>(base + state_off).value_or(0);
                uint32_t key   = key38 ? key38 : key0;

                static size_t xor_diag_logs = 0;
                if (xor_diag_logs < memory_map::UWorldStateCandidates.size() + 3) {
                    LOG_INFO(std::format(
                        "[DIAG XOR] base=0x{:X} | state_off=0x{:X} | "
                        "state_addr=0x{:X} | key38=0x{:X} | key0=0x{:X}",
                        base,
                        state_off,
                        base + state_off,
                        key38,
                        key0
                    ));
                    ++xor_diag_logs;
                }
                if (key) {
#pragma pack(push, 1)
                    struct State {
                        uintptr_t keys[memory_map::derive::UObjectStateSlots];
                    };
#pragma pack(pop)
                    static_assert(sizeof(State) == sizeof(uintptr_t) * memory_map::derive::UObjectStateSlots,
                                  "State slot count must match memory_map::derive::UObjectStateSlots");
                    State state = maxi::driver::g_driver->read<State>(base + (key38 ? state_off : state_off + memory_map::StateKeyAltShift));
                    model::UObject dummy;
                    uintptr_t decrypted_ptr = dummy.decrypt_xor_keys(key, reinterpret_cast<uintptr_t*>(&state));
                     if (decrypted_ptr) {
                         uintptr_t world_ptr = maxi::driver::g_driver->try_read<uintptr_t>(decrypted_ptr).value_or(0);



                         if (world_ptr && world_ptr > 0x10000 && world_ptr < base) {
                             uintptr_t final_world = world_ptr;
                             uintptr_t inner = maxi::driver::g_driver->try_read<uintptr_t>(final_world).value_or(0);
                             if (inner && inner > 0x10000 && inner < base && inner != final_world && level_present(inner))
                                 final_world = inner;
                             if (level_present(final_world)) {
                                 LOG_INFO(std::format("[DIAG SUCCESS] Decrypted UWorld -> 0x{:X}", final_world));
                                 return final_world;
                             }
                         }
                     }
                }
            }


            uintptr_t last_valid_world = 0;

            for (uintptr_t gw_off : memory_map::GWorldStaticCandidates) {
                if (!gw_off) continue;
                uintptr_t direct_world = maxi::driver::g_driver->try_read<uintptr_t>(base + gw_off).value_or(0);
                static size_t static_scan_logs = 0;
                if (static_scan_logs < memory_map::GWorldStaticCandidates.size()) {
                    LOG_INFO(std::format("[DIAG SCAN] base: 0x{:X}, gw_off: 0x{:X}, direct_world: 0x{:X}", base, gw_off, direct_world));
                    ++static_scan_logs;
                }
                if (gw_off == memory_map::GWorld) {
                    static uint64_t gworld_frame = 0;
                    if ((++gworld_frame % 200) == 0) {
                        LOG_INFO(std::format("[GWORLD] direct_world=0x{:X} dtb=0x{:X}", direct_world, maxi::driver::g_driver->get_dtb()));
                    }
                }
                if (direct_world && direct_world > 0x10000 && direct_world < base) {


                    uintptr_t step1 = direct_world;

                    static uintptr_t s_last_logged_w = 0;
                    const uintptr_t prev_logged = s_last_logged_w;



                    uintptr_t step2 = maxi::driver::g_driver->try_read<uintptr_t>(step1).value_or(0);
                    if (step2 && step2 > 0x10000 && step2 < base && step2 != step1 && level_present(step2)) {
                        if (step2 != prev_logged) {
                            s_last_logged_w = step2;
                            LOG_INFO(std::format("[DIAG SUCCESS] GWorld deref UWorld -> 0x{:X} (slot 0x{:X} = 0x{:X})", step2, gw_off, step1));
                        }
                        s_static_gworld_ok = true;
                        return step2;
                    }


                    if (level_present(step1)) {
                        if (step1 != prev_logged) {
                            s_last_logged_w = step1;
                            LOG_INFO(std::format("[DIAG SUCCESS] GWorld Resolved UWorld -> 0x{:X} via offset 0x{:X}", step1, gw_off));
                        }
                        s_static_gworld_ok = true;
                        return step1;
                    }

                    if (last_valid_world == 0 || last_valid_world == step1) {
                        last_valid_world = step1;
                    }
                }
            }


            if (last_valid_world != 0) {
                if (level_present(last_valid_world)) {
                    LOG_INFO(std::format("[DIAG SUCCESS] GWorld Resolved UWorld (fallback) -> 0x{:X}", last_valid_world));
                    s_static_gworld_ok = true;
                    return last_valid_world;
                }
            }
        }


        if (m_kernel_protocol) {
            auto pml4 = m_kernel_protocol->find_pml4_base();
            if (pml4.DecryptedClonedCr3 && pml4.PML4Base) {
                auto cached_off = m_uworld_offset.load();
                if (cached_off) {
                    auto w_ptr = maxi::driver::g_driver->try_read<uintptr_t>(pml4.PML4Base + cached_off).value_or(0);
                    if (w_ptr && w_ptr > 0x10000) return w_ptr;
                    m_uworld_offset.store(0);
                }

                for (int i = 0; i < 0x1000; i += 8) {
                    const auto world_ptr = maxi::driver::g_driver->try_read<uintptr_t>(pml4.PML4Base + i).value_or(0);
                    if (!world_ptr || world_ptr < 0x10000) continue;

                    const auto lvl = maxi::driver::g_driver->try_read<uintptr_t>(world_ptr + memory_map::PersistentLevel).value_or(0);
                    if (!lvl || lvl == world_ptr || lvl < 0x10000) continue;

                    const auto owning_world = maxi::driver::g_driver->try_read<uintptr_t>(lvl + memory_map::OwningWord).value_or(0);
                    if (owning_world == world_ptr) {
                        m_uworld_offset.store(i);
                        static bool logged_pml4 = false;
                        if (!logged_pml4) {
                            LOG_INFO(std::format("[DIAG SUCCESS] PML4 Resolved UWorld -> 0x{:X} (Level: 0x{:X}, OwningWorld: 0x{:X})", world_ptr, lvl, owning_world));
                            logged_pml4 = true;
                        }
                        return world_ptr;
                    }
                }
            }
        }


        if (base) {
            auto world_base_ptr = maxi::driver::g_driver->try_read<uintptr_t>(base + memory_map::GWorld).value_or(0);
            if (world_base_ptr && world_base_ptr > 0x10000) {

                uintptr_t final_world = world_base_ptr;
                if (level_present(final_world)) {
                    LOG_INFO(std::format("[DIAG SUCCESS] Guarded Region UWorld -> 0x{:X}", final_world));
                    return final_world;
                }
                auto world_ptr = maxi::driver::g_driver->try_read<uintptr_t>(final_world).value_or(0);
                if (world_ptr && world_ptr > 0x10000 && world_ptr != final_world && level_present(world_ptr)) {
                    LOG_INFO(std::format("[DIAG SUCCESS] Guarded Region UWorld (deref) -> 0x{:X}", world_ptr));
                    return world_ptr;
                }
            }
        }

        static bool logged_fail = false;
        if (!logged_fail) {
            LOG_WARNING(std::format("[DIAG FAIL] resolve_world_address failed. base: 0x{:X}", base));
            logged_fail = true;
        }

        return 0;
    }

    void CoreEngine::camera_thread_loop() {
        while (m_running.load()) {
            maxi::driver::g_read_cat = 2;
            try {
                if (maxi::driver::g_driver && maxi::driver::g_driver->get_process_id() != 0) {
                    auto wd = get_world_data();
                    if (wd.player_camera_manager.is_valid())
                        refresh_camera_cache(wd.player_camera_manager);
                }
            }
            catch (const std::exception& e) {
                LOG_ERROR(std::format("Exception in camera thread: {}", e.what()));
            }
            Sleep(10);
        }
    }

    void CoreEngine::world_thread_loop() {
        while (m_running.load()) {
            maxi::driver::g_read_cat = 1;
            try {
                if (!maxi::driver::g_driver || maxi::driver::g_driver->get_process_id() == 0) {
                    set_target_process(maxi::driver::target_process_name_w());
                    Sleep(500);
                    continue;
                }
                update_world_data();
            }
            catch (const std::exception& e) {
                LOG_ERROR(std::format("Exception in world thread: {}", e.what()));
            }
            Sleep(200);
        }
    }

    void CoreEngine::actors_thread_loop() {
        while (m_running.load()) {
            maxi::driver::g_read_cat = 3;
            try {
                update_actors_data();
            }
            catch (const std::exception& e) {
                LOG_ERROR(std::format("Exception in actors thread: {}", e.what()));
            }


            {
                static ULONGLONG s_cats_t = 0;
                static uint64_t s_cats_n[9]{}, s_cats_b[9]{}, s_cats_ns[9]{};
                const ULONGLONG now = GetTickCount64();
                if (now - s_cats_t >= 10000) {
                    s_cats_t = now;
                    auto catf = [&](int c, const char* nm, std::string& out) {
                        const uint64_t n = maxi::driver::g_cat_n[c].load();
                        const uint64_t b = maxi::driver::g_cat_bytes[c].load();
                        const uint64_t ns = maxi::driver::g_cat_ns[c].load();
                        const uint64_t mx = maxi::driver::g_cat_max[c].load();
                        const uint64_t dn = n - s_cats_n[c]; s_cats_n[c] = n;
                        const uint64_t db = b - s_cats_b[c]; s_cats_b[c] = b;
                        const uint64_t dtn = ns - s_cats_ns[c]; s_cats_ns[c] = ns;
                        char buf[128];
                        snprintf(buf, sizeof(buf), "%s:n=%llu/b=%llu/avg=%.2fms/max=%.2fms ",
                            nm, (unsigned long long)dn, (unsigned long long)db,
                            dn ? (double)dtn / (double)dn / 1e6 : 0.0,
                            (double)mx / 1e6);
                        out += buf;
                    };
                    std::string tbl;
                    catf(1, "W", tbl); catf(2, "C", tbl); catf(3, "A", tbl);
                    catf(4, "B", tbl); catf(6, "X", tbl); catf(8, "S", tbl);
                    LOG_INFO(std::format("[CATS] {} | remaps={}", tbl, g_backend_remaps));
                }
            }
            Sleep(95 + (GetTickCount64() % 35));
        }
    }

    void CoreEngine::skills_thread_loop() {



        while (m_running.load()) {
            Sleep(500);
        }
    }



    struct CatGuard {
        int prev;
        explicit CatGuard(int c) : prev(maxi::driver::g_read_cat) { maxi::driver::g_read_cat = c; }
        ~CatGuard() { maxi::driver::g_read_cat = prev; }
    };

    void CoreEngine::bones_thread_loop() {

        static uint64_t s_ph_r = 0;
        static uint64_t s_bytes = 0;
        static uint64_t s_wbfail = 0;

        static uint64_t s_lc4 = 0, s_lcb4 = 0;
        static uint64_t s_jbfail = 0;
        static uint64_t s_batt = 0, s_bok = 0, s_bbad = 0;
        static uint64_t s_synth_runs = 0, s_rootfail = 0;
        static uint64_t s_sk_hp = 0, s_sk_dead = 0, s_sk_noba = 0, s_jobs = 0;
        static double s_readms_sum = 0.0; static uint64_t s_readms_n = 0;
        static uint64_t s_same = 0, s_chg = 0;
        static std::unordered_map<uintptr_t, int> s_qfail;
        static std::unordered_map<uintptr_t, ULONGLONG> s_quar;

        static std::unordered_map<uintptr_t, int> s_meshcnt;
        static std::unordered_map<int, model::FVector> s_headdC;
        static std::unordered_map<int, model::FVector> s_footdC;
        auto mesh_count = [](uintptr_t mesh) -> int {
            if (!mesh) return 0;
            auto itc = s_meshcnt.find(mesh);
            if (itc != s_meshcnt.end()) return itc->second;
            int c = 0;
            if (auto v = maxi::driver::g_driver->try_read<int32_t>(mesh + memory_map::BoneArray + memory_map::BoneCountOffset); v && *v > 0 && *v <= 512) c = *v;
            else if (auto v2 = maxi::driver::g_driver->try_read<int32_t>(mesh + memory_map::BoneArrayCache + memory_map::BoneCountOffset); v2 && *v2 > 0 && *v2 <= 512) c = *v2;
            if (s_meshcnt.size() > 64) s_meshcnt.clear();
            s_meshcnt[mesh] = c;
            return c;
        };
        static double s_cyc_max = 0.0;
        static double s_perr_sum = 0.0, s_perr_max = 0.0;
        static uint64_t s_perr_n = 0;
        static double s_spd_sum = 0.0, s_dvel_sum = 0.0;
        while (m_running.load()) {
            if (m_paused.load()) {
                Sleep(10);
                continue;
            }
            const auto cyc_start = std::chrono::steady_clock::now();
            maxi::driver::g_read_cat = 4;
            try {

                static size_t s_bones_rr = 0;

                constexpr size_t BONES_BATCH = 16;
                const size_t total = m_actors_data.players_list.size();
                if (total > 0) {
                    size_t base = s_bones_rr % total;


                    struct BoneJob { size_t idx; uintptr_t base_addr; uintptr_t actor; uintptr_t mesh; uintptr_t bone_array; float health; bool name_unknown; uintptr_t dh; bool vis; uint64_t health_ms; std::string dbg_name; };
                    std::array<BoneJob, BONES_BATCH> jobs{};
                    size_t njobs = 0;
                    {
                        if (!m_actors_data_mutex.try_lock()) { ++s_jbfail; Sleep(1); continue; }
                        auto& list = m_actors_data.players_list;
                        auto push_job = [&](size_t i) {
                            auto& info = list[i];
                            const bool unk = (info.name == "??" || info.name.empty() || info.name == "Bot");
                            jobs[njobs++] = { i, info.actor.get_base_address(), info.actor.get_base_address(), info.mesh, info.bone_array, info.health, unk, info.damage_handler.get_base_address(), info.visible, info.health_ms, info.name };
                        };
                        for (size_t k = 0; k < total && njobs < BONES_BATCH; ++k) {
                            size_t i = (base + k) % total;
                            const auto& nm = list[i].name;
                            if (nm != "??" && !nm.empty()) push_job(i);
                        }
                        for (size_t k = 0; k < total && njobs < BONES_BATCH; ++k) {
                            size_t i = (base + k) % total;
                            const auto& nm = list[i].name;
                            if (nm == "??" || nm.empty()) push_job(i);
                        }
                        m_actors_data_mutex.unlock();
                    }
                    s_jobs += njobs;


                    static size_t s_aux_cycle = 0;




                    const bool refresh_aux = (s_aux_cycle++ % 2) == 0;
                    static uint64_t s_hcyc = 0;
                    const uint64_t hcyc = s_hcyc++;
                    struct BoneResult { std::array<model::FVector,17> bones; bool bones_ok; float health; bool health_fresh; int32_t team; bool ally; bool vis; std::string name; uintptr_t mesh; uintptr_t bone_array; bool ptrs_updated; bool hide; bool hide_eval; uintptr_t dh; uint64_t read_ms; bool root_ok; model::FVector root; bool dorm; float yaw; bool hasyaw; const char* dbg; };
                    std::array<BoneResult, BONES_BATCH> res{};
                    int s_names_this_cycle = 0;
                    const auto _tr0 = std::chrono::steady_clock::now();
                    for (size_t k = 0; k < njobs; ++k) {
                        auto& j = jobs[k];
                        res[k].bones_ok = false;
                        res[k].ptrs_updated = false;
                        res[k].hide = false;
                        res[k].hide_eval = false;
                        res[k].mesh = 0; res[k].bone_array = 0; res[k].dh = 0; res[k].read_ms = 0; res[k].root_ok = false; res[k].dorm = false; res[k].hasyaw = false; res[k].dbg = "?";
                        res[k].health_fresh = false;
                        res[k].health = j.health;
                        res[k].vis = j.vis;
                        auto a = model::AActor(j.actor);
                        std::optional<float> health_opt;
                        uintptr_t dh_used = j.dh;
                        if (j.health <= 0.f && j.health_ms != 0 &&
                            GetTickCount64() - j.health_ms < 5000) {
                            ++s_sk_dead;
                            res[k].dbg = "deadc";
                            continue;
                        }

                        static std::unordered_map<uintptr_t, ULONGLONG> s_hpcd;
                        bool hp_skip = false;
                        {
                            const ULONGLONG hn = GetTickCount64();
                            auto hit = s_hpcd.find(jobs[k].base_addr);
                            if (hit != s_hpcd.end() && hn - hit->second < 1000) hp_skip = true;
                        }
                        const bool health_turn = !hp_skip && (((hcyc + k) % 2) == 0);
                        if (health_turn) {
                            if (dh_used) {
                            health_opt = model::ADamageHandler(dh_used).TryGetHealth();
                            if (!health_opt) {
                                dh_used = 0;
                                if (auto fresh = a.TryDamageHandler()) {
                                    dh_used = fresh->get_base_address();
                                    if (dh_used) health_opt = fresh->TryGetHealth();
                                    else dh_used = 0;
                                }
                            }
                        } else {
                            if (auto fresh = a.TryDamageHandler()) {
                                dh_used = fresh->get_base_address();
                                if (dh_used) health_opt = fresh->TryGetHealth();
                                else dh_used = 0;
                            }
                        }
                        res[k].dh = dh_used;
                            if (!health_opt) {
                                if (s_hpcd.size() > 512) s_hpcd.clear();
                                s_hpcd[jobs[k].base_addr] = GetTickCount64();
                                ++s_sk_hp; res[k].dbg = "hp"; continue;
                            }
                            s_hpcd.erase(jobs[k].base_addr);
                            res[k].health = *health_opt;
                            res[k].health_fresh = true;
                        } else {
                            res[k].health = j.health;
                        }
                        static std::unordered_map<uintptr_t, uint8_t> s_hdead;
                        if (res[k].health_fresh) {
                            if (res[k].health <= 0.f) {
                                if (++s_hdead[jobs[k].base_addr] < 2) {
                                    res[k].health = 100.f;
                                    res[k].health_fresh = false;
                                }
                            } else {
                                s_hdead.erase(jobs[k].base_addr);
                            }
                        }
                        if (res[k].health <= 0.f) { ++s_sk_dead; res[k].dbg = "dead"; continue; }

                        if (!j.bone_array) {


                            static std::unordered_map<uintptr_t, ULONGLONG> s_nobacd;
                            {
                                const ULONGLONG nn = GetTickCount64();
                                auto nit = s_nobacd.find(jobs[k].base_addr);
                                if (nit != s_nobacd.end() && nn - nit->second < 3000) {
                                    ++s_sk_noba; res[k].dbg = "noba"; continue;
                                }
                            }
                            uintptr_t b2 = 0;
                            auto m_opt = a.TryResolveMesh(b2);
                            if (m_opt && *m_opt && b2) {
                                j.mesh = *m_opt; j.bone_array = b2;
                                res[k].mesh = *m_opt; res[k].bone_array = b2; res[k].ptrs_updated = true;
                                s_nobacd.erase(jobs[k].base_addr);
#ifdef _DEBUG

                                if (!model::AActor::MeshHasBodyBone(*m_opt)) {
                                    static std::unordered_set<uintptr_t> s_ms;
                                    if (s_ms.insert(jobs[k].base_addr).second) {
                                        std::string cs;
                                        for (uintptr_t mo : { memory_map::Mesh, memory_map::MeshCharacter, memory_map::Mesh1P, memory_map::MeshOverlay1P }) {
                                            auto mv = maxi::driver::g_driver->try_read<uintptr_t>(jobs[k].base_addr + mo).value_or(0);
                                            int bcv = mv ? a.BoneCount(mv) : 0;
                                            bool bod = mv && model::AActor::MeshHasBodyBone(mv);
                                            cs += std::format(" +0x{:X}=0x{:X}(bc={},body={})", (unsigned long long)mo, (unsigned long long)mv, bcv, bod ? 1 : 0);
                                        }
                                        LOG_WARNING(std::format("[MESHSCAN] name={} sel=+mesh0x{:X}{}", res[k].name.empty() ? "??" : res[k].name, (unsigned long long)0, cs));
                                    }
                                }
#endif
                            } else {
                                if (s_nobacd.size() > 512) s_nobacd.clear();
                                s_nobacd[jobs[k].base_addr] = GetTickCount64();
                                ++s_sk_noba; res[k].dbg = "noba"; continue;
                            }
                        }

                        if (j.mesh && !model::AActor::MeshHasBodyBone(j.mesh)) {
                            for (uintptr_t mo : { memory_map::Mesh, memory_map::MeshCharacter }) {
                                auto mv = maxi::driver::g_driver->try_read<uintptr_t>(jobs[k].base_addr + mo);
                                if (!mv || !*mv || *mv == j.mesh) continue;
                                if (!model::AActor::MeshHasBodyBone(*mv)) continue;
                                uintptr_t nb = maxi::driver::g_driver->try_read<uintptr_t>(*mv + memory_map::BoneArray).value_or(0);
                                if (!nb) nb = maxi::driver::g_driver->try_read<uintptr_t>(*mv + memory_map::BoneArrayCache).value_or(0);
                                if (!nb) continue;
                                j.mesh = *mv; j.bone_array = nb;
                                res[k].mesh = *mv; res[k].bone_array = nb; res[k].ptrs_updated = true;
                                break;
                            }
                        }

                        static std::unordered_map<uintptr_t, ULONGLONG> s_healcd;
                        bool heal_ok = true;
                        {
                            const ULONGLONG hnow = GetTickCount64();
                            auto hhit = s_healcd.find(j.bone_array);
                            if (hhit != s_healcd.end() && hnow - hhit->second < 5000) heal_ok = false;
                            else {
                                if (s_healcd.size() > 256) s_healcd.clear();
                                s_healcd[j.bone_array] = hnow;
                            }
                        }
                        bool qskip = false;
                        {
                            auto itq = s_quar.find(j.bone_array);
                            if (itq != s_quar.end()) {

                                qskip = false;
                                s_quar.erase(itq); s_qfail.erase(j.bone_array);
                            }
                        }
                        std::optional<std::array<model::FVector, 17>> bones_opt;
                        if (!qskip) {
                            bones_opt = GetActorBoneWorldPositions(a, j.mesh, j.bone_array);
                            ++s_batt;
                        }
                        if (!bones_opt && j.mesh && s_qfail[j.bone_array] < 4) {
                            if (auto bcq = maxi::driver::g_driver->try_read<uintptr_t>(j.mesh + memory_map::BoneArrayCache)) {
                                const uintptr_t bq = *bcq;
                                if (bq && bq != j.bone_array) {
                                    if (auto boq = GetActorBoneWorldPositions(a, j.mesh, bq)) {
                                        bones_opt = std::move(boq);
                                        res[k].bone_array = bq; res[k].ptrs_updated = true;
                                        res[k].read_ms = GetTickCount64();
                                    }
                                }
                            }
                        }
                        if (!bones_opt && heal_ok && !qskip) {
                            CatGuard _cg8(8);
                            uintptr_t b2 = 0;
                            auto m_res = a.TryResolveMesh(b2);
                            const uintptr_t m2 = (m_res && *m_res) ? *m_res : 0;
                            if (m2 && b2 && (m2 != j.mesh || b2 != j.bone_array)) {
                                res[k].mesh = m2; res[k].bone_array = b2; res[k].ptrs_updated = true;
                                bones_opt = GetActorBoneWorldPositions(a, m2, b2);
                            }
                        }
                        if (bones_opt) {
                            ++s_bok;
                            s_qfail.erase(j.bone_array);
                            res[k].bones = *bones_opt;
                            res[k].bones_ok = true;
                            res[k].dbg = "ok";
                            res[k].read_ms = GetTickCount64();
                        } else {
                            ++s_bbad;
                            res[k].dbg = "bonesfail";
                            ++s_qfail[j.bone_array];
                            if (auto rc = maxi::driver::g_driver->try_read<uintptr_t>(j.actor + memory_map::RootComponent); rc && *rc) {
                                if (auto loc = maxi::driver::g_driver->try_read<model::FVector>(*rc + memory_map::RelativeLocation)) {
                                    if (std::isfinite(loc->X) && std::isfinite(loc->Y) && std::isfinite(loc->Z) &&
                                        std::abs(loc->X) < 100000.0 && std::abs(loc->Y) < 100000.0 && std::abs(loc->Z) < 100000.0) {
                                        res[k].root = *loc;
                                        res[k].root_ok = true;
                                    }
                                }
                            }
                        }

#ifdef _DEBUG

                        static int s_nob_total = 0;
                        if (!res[k].bones_ok && j.bone_array && health_turn && s_nob_total < 60) {
                            static std::unordered_set<uintptr_t> s_nob;
                            if (s_nob.insert(jobs[k].base_addr).second) {
                                ++s_nob_total;
                                int32_t bcc = 0;
                                try { bcc = a.BoneCount(j.mesh); } catch (...) { bcc = -1; }
                                std::string nn = "??";
                                if (m_actors_data_mutex.try_lock()) {
                                    for (auto& ex : m_actors_data.players_list)
                                        if (ex.actor.get_base_address() == jobs[k].base_addr) { nn = ex.name; break; }
                                    m_actors_data_mutex.unlock();
                                }
                                const char* why = !res[k].health_fresh ? "nohealth"
                                    : (res[k].health <= 0.f ? "dead" : "bonesfail");


                                int dormv = -1;
                                if (auto dd = maxi::driver::g_driver->try_read<uint8_t>(jobs[k].base_addr + 0x100)) dormv = (int)*dd;
                                const char* rootq = "null";
                                std::optional<model::FVector> rootv;
                                if (auto rc = maxi::driver::g_driver->try_read<uintptr_t>(jobs[k].base_addr + memory_map::RootComponent); rc && *rc) {
                                    auto loc = maxi::driver::g_driver->try_read<model::FVector>(*rc + memory_map::RelativeLocation);
                                    if (!loc) rootq = "fail";
                                    else if (!std::isfinite(loc->X) || !std::isfinite(loc->Y) || !std::isfinite(loc->Z) ||
                                        std::abs(loc->X) > 100000.0 || std::abs(loc->Y) > 100000.0 || std::abs(loc->Z) > 100000.0) rootq = "garb";
                                    else { rootq = "ok"; rootv = *loc; }
                                }


                                double distm = -1.0;
                                if (rootv) {
                                    auto wd0 = get_world_data();
                                    const uintptr_t pawn = wd0.acknowledged_pawn.get_base_address();
                                    if (pawn) {
                                        if (auto prc = maxi::driver::g_driver->try_read<uintptr_t>(pawn + memory_map::RootComponent); prc && *prc) {
                                            if (auto ploc = maxi::driver::g_driver->try_read<model::FVector>(*prc + memory_map::RelativeLocation)) {
                                                const double dx = rootv->X - ploc->X, dy = rootv->Y - ploc->Y, dz = rootv->Z - ploc->Z;
                                                if (std::isfinite(dx) && std::isfinite(dy) && std::isfinite(dz))
                                                    distm = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0;
                                            }
                                        }
                                    }
                                }
                                auto dhx = maxi::driver::g_driver->try_read<uintptr_t>(jobs[k].base_addr + memory_map::DamageHandler).value_or(0);
                                float dhh = -99999.f;
                                if (dhx) dhh = maxi::driver::g_driver->try_read<float>(dhx + memory_map::Health).value_or(-99999.f);


                                auto ba1 = maxi::driver::g_driver->try_read<uintptr_t>(j.mesh + memory_map::BoneArray).value_or(0);
                                auto ba2 = maxi::driver::g_driver->try_read<uintptr_t>(j.mesh + memory_map::BoneArrayCache).value_or(0);
                                auto slotq = [&](uintptr_t ba) -> std::string {
                                    if (!ba) return "null";
                                    auto t = maxi::driver::g_driver->try_read<model::FTransform>(ba);
                                    if (!t) return "fail";
                                    const auto& v = t->Translation;
                                    if (!std::isfinite(v.X) || !std::isfinite(v.Y) || !std::isfinite(v.Z) ||
                                        std::abs(v.X) > 100000.0 || std::abs(v.Y) > 100000.0 || std::abs(v.Z) > 100000.0)
                                        return "garb";
                                    return std::format("ok({:.0f},{:.0f},{:.0f})", v.X, v.Y, v.Z);
                                };
                                LOG_WARNING(std::format("[NOBONES] name={} bc={} why={} dh58=0x{:X}(h200={:.0f}) ba=0x{:X} used=0x{:X} ba1=0x{:X}({}) ba2=0x{:X}({}) dorm100={} root={} dist={:.0f}m",
                                    nn, bcc, why, dhx, dhh, (unsigned long long)jobs[k].base_addr, j.bone_array, ba1, slotq(ba1), ba2, slotq(ba2), dormv, rootq, distm));

                                {
                                    static std::unordered_set<uintptr_t> s_mf;
                                    if (s_mf.insert(jobs[k].base_addr).second) {
                                        for (uintptr_t mo : { (uintptr_t)0xF10, (uintptr_t)0xF18, (uintptr_t)0x4F0 }) {
                                            auto mm = maxi::driver::g_driver->try_read<uintptr_t>(jobs[k].base_addr + mo);
                                            if (!mm || !*mm || *mm == j.mesh) continue;
                                            auto bb = maxi::driver::g_driver->try_read<uintptr_t>(*mm + memory_map::BoneArray);
                                            if (!bb || !*bb) continue;
                                            if (auto ok = GetActorBoneWorldPositions(a, *mm, *bb)) {
                                                LOG_WARNING(std::format("[MESHFIX] ba=0x{:X} altmesh=+0x{:X}=0x{:X} altba=0x{:X} VALID",
                                                    (unsigned long long)jobs[k].base_addr, mo,
                                                    (unsigned long long)*mm, (unsigned long long)*bb));
                                                break;
                                            }
                                        }
                                    }
                                }
                                {
                                    size_t maxkb = 0;
                                    double zeroratio = -1.0;
                                    for (size_t kb : { (size_t)9, (size_t)4, (size_t)1 }) {
                                        const size_t sz = kb * 1024u;
                                        std::vector<std::uint8_t> tb(sz);
                                        ++maxi::driver::g_read_calls;
                                        maxi::driver::g_read_bytes += sz;
                                        if (maxi::driver::g_driver->read_memory(j.bone_array, tb.data(), sz)) {
                                            maxkb = kb;
                                            if (kb == 9 && bcc > 0 && bcc <= 512) {
                                                size_t zeros = 0;
                                                const size_t stride = memory_map::BoneStride ? memory_map::BoneStride : sizeof(model::FTransform);
                                                const size_t n = (std::min)((size_t)bcc, sz / stride);
                                                for (size_t zi = 0; zi < n; ++zi) {
                                                    model::FTransform t{};
                                                    std::memcpy(&t, tb.data() + zi * stride, sizeof(t));
                                                    static const uint64_t z96[12] = {};
                                                    if (std::memcmp(&t, z96, sizeof(t)) == 0) ++zeros;
                                                }
                                                if (n) zeroratio = 100.0 * (double)zeros / (double)n;
                                            }
                                            break;
                                        }
                                    }
                                    LOG_WARNING(std::format("[BULKSZ] ba=0x{:X} maxkb={} zero=%{:.0f}",
                                        (unsigned long long)j.bone_array, maxkb, zeroratio));
                                }
                            }
                        }
#endif
                        if (refresh_aux) {
                            CatGuard _cg(6);

                            res[k].team = -1;
                            
                            auto ally_opt = a.TryWasAlly();
                            res[k].ally = ally_opt.value_or(false);





                            {
                                auto vis_opt = a.TryIsVisible(j.mesh);
                                res[k].vis = vis_opt.value_or(false);
                            }

                            if (!res[k].name.empty() && res[k].name != "??") {
                                static std::unordered_set<uintptr_t> s_vd;
                                if (s_vd.insert(jobs[k].base_addr).second) {
                                    auto mm = a.TryMinimapComponent();
                                    const uintptr_t mmb = (mm && mm->is_valid()) ? mm->get_base_address() : 0;
                                    uintptr_t vcb = 0;
                                    if (mmb) { auto vc = mm->TryVisibilityComponent(); if (vc && vc->is_valid()) vcb = vc->get_base_address(); }
                                    const bool bv = mmb ? mm->bVisible() : false;
                                    auto lrt = maxi::driver::g_driver->try_read<float>(j.mesh + memory_map::LastRenderTime);
                                    auto lst = maxi::driver::g_driver->try_read<float>(j.mesh + memory_map::LastSubmitTime);
                                    LOG_WARNING(std::format("[VISDIAG] name={} vis={} mm=0x{:X} vc=0x{:X} bVis={} lrt={:.2f} lst={:.2f}",
                                        res[k].name, res[k].vis ? 1 : 0, (unsigned long long)mmb, (unsigned long long)vcb,
                                        bv ? 1 : 0, lrt.value_or(-1.f), lst.value_or(-1.f)));
                                }
                            }



                            if (var->config.visuals.ignore_dormant)
                                res[k].dorm = maxi::driver::g_driver->try_read<uint8_t>(j.actor + 0x100).value_or(0) != 0;



                            if (var->config.visuals.view_angle) {
                                if (auto rc = maxi::driver::g_driver->try_read<uintptr_t>(j.actor + memory_map::RootComponent); rc && *rc) {
                                    if (auto rt = maxi::driver::g_driver->try_read<model::FRotator>(*rc + memory_map::RelativeRotation)) {
                                        res[k].yaw = (float)rt->Yaw;
                                        res[k].hasyaw = true;
                                    }
                                }
                            }



                            if (s_names_this_cycle < 16 && j.name_unknown) {
                                std::string name_str = a.get_name_string();
                                std::string agent_name = GetAgentName(name_str);
                                bool is_bot = false;
                                if (!agent_name.empty() && (agent_name == "Bot" || agent_name == "Bot Training")) is_bot = true;
                                else if (agent_name.empty() && !name_str.empty() &&
                                    (name_str.find("Character") != std::string::npos || name_str.find("Pawn") != std::string::npos ||
                                     name_str.find("Bot") != std::string::npos || name_str.find("Dummy") != std::string::npos)) {
                                    agent_name = "Bot";
                                    is_bot = true;
                                }
                                res[k].hide_eval = true;
                                if (is_bot && !var->config.visuals.show_bots) {
                                    res[k].hide = true;
                                } else {
                                    res[k].hide = false;
                                    if (!agent_name.empty()) {
                                        const int32_t fk = a.get_name();
                                        if (fk != 0) maxi::game::model::get_global_fname_cache().put(fk, agent_name);
                                        res[k].name = agent_name;
                                    }
#ifdef _DEBUG
                                    else {
                                        const int32_t fk = a.get_name();
                                        static std::unordered_set<int32_t> s_nnlog;
                                        if (s_nnlog.insert(fk).second) {
                                            std::string rs = name_str.size() > 48 ? name_str.substr(0, 48) : name_str;
                                            LOG_WARNING(std::format("[NONAME] fk={} raw='{}'",
                                                 fk, rs.empty() ? "<empty>" : rs));
                                        }
                                    }
#endif
                                }
                                ++s_names_this_cycle;
                            }


#ifdef _DEBUG
                            {
                                static int s_tdiag_n = 0;
                                if (s_tdiag_n < 3 && !res[k].name.empty() && res[k].name != "??") {
                                    auto tr_u64 = [&](uintptr_t ad) -> uint64_t {
                                        auto o = maxi::driver::g_driver->try_read<uint64_t>(ad);
                                        return o.value_or(0);
                                    };
                                    auto tr_i32 = [&](uintptr_t ad) -> int32_t {
                                        auto o = maxi::driver::g_driver->try_read<int32_t>(ad);
                                        return o.value_or(-999);
                                    };
                                    auto tr_u8 = [&](uintptr_t ad) -> int {
                                        auto o = maxi::driver::g_driver->try_read<uint8_t>(ad);
                                        return o ? (int)*o : -1;
                                    };
                                    const uintptr_t tcA = tr_u64(j.actor + 0x680);
                                    const uintptr_t tcB = tr_u64(j.actor + memory_map::TeamComponent);
                                    double idAd = -999.0, idBd = -999.0;
                                    if (tcA) { auto o = maxi::driver::g_driver->try_read<double>(tcA + memory_map::TeamId); if (o) idAd = *o; }
                                    if (tcB) { auto o = maxi::driver::g_driver->try_read<double>(tcB + memory_map::TeamId); if (o) idBd = *o; }
                                    int32_t diag_bc = 0;
                                    try { diag_bc = a.BoneCount(j.mesh); } catch (...) { diag_bc = -1; }
                                    auto dh50 = maxi::driver::g_driver->try_read<uintptr_t>(j.actor + 0xC50).value_or(0);
                                    auto dh58 = maxi::driver::g_driver->try_read<uintptr_t>(j.actor + memory_map::DamageHandler).value_or(0);
                                    auto rd_f = [&](uintptr_t ad) -> float {
                                        auto o = maxi::driver::g_driver->try_read<float>(ad);
                                        return o ? *o : -99999.f;
                                    };
                                    LOG_WARNING(std::format("[TEAMDIAG] name={} bc={} hp={:.0f} vis={} tcA=0x{:X} tcB=0x{:X} idA32={} idAd={:.0f} idB32={} idBd={:.0f} ally={} uid38={} dorm100={} dorm225={} dh50=0x{:X}(h1E0={:.0f},h200={:.0f}) dh58=0x{:X}(h1E0={:.0f},h200={:.0f})",
                                        res[k].name, diag_bc, res[k].health, res[k].vis ? 1 : 0, tcA, tcB,
                                        tcA ? tr_i32(tcA + memory_map::TeamId) : -999, idAd,
                                        tcB ? tr_i32(tcB + memory_map::TeamId) : -999, idBd,
                                        res[k].ally ? 1 : 0,
                                        tr_i32(j.actor + memory_map::UniqueID),
                                        tr_u8(j.actor + 0x100), tr_u8(j.actor + 0x225),
                                        dh50, rd_f(dh50 ? dh50 + 0x1E0 : 0), rd_f(dh50 ? dh50 + 0x200 : 0),
                                        dh58, rd_f(dh58 ? dh58 + 0x1E0 : 0), rd_f(dh58 ? dh58 + 0x200 : 0)));
                                    ++s_tdiag_n;
                                }
                            }
#endif
                        }
                    }
                    static bool s_team_diag2 = false;
                    if (!s_team_diag2 && njobs > 0 && m_actors_data_mutex.try_lock()) {
                        s_team_diag2 = true;
                        std::string dname = "??";
                        if (jobs[0].idx < m_actors_data.players_list.size())
                            dname = m_actors_data.players_list[jobs[0].idx].name;
                        m_actors_data_mutex.unlock();
                        LOG_INFO(std::format("[TEAM DIAG2] name={} wasAlly={} team_id={} visible={}",
                            dname,
                            res[0].ally ? 1 : 0, res[0].team, res[0].vis ? 1 : 0));
                    }
                    const double _readms = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - _tr0).count();
                    s_readms_sum += _readms; s_readms_n++;
                    bool wb_locked = m_actors_data_mutex.try_lock();
                    if (!wb_locked) { Sleep(1); wb_locked = m_actors_data_mutex.try_lock(); }
                    if (wb_locked) {
                        auto& list = m_actors_data.players_list;
                        for (size_t k = 0; k < njobs; ++k) {
                            for (auto it = list.begin(); it != list.end(); ++it) {
                                if (it->actor.get_base_address() != jobs[k].base_addr) continue;

                                if (res[k].ptrs_updated) { it->mesh = res[k].mesh; it->bone_array = res[k].bone_array; }

                                if (res[k].dh) it->damage_handler = model::ADamageHandler(res[k].dh);




                                if (res[k].health_fresh) {
                                    it->health = res[k].health;
                                    it->health_ms = GetTickCount64();
                                }

                                if (res[k].health_fresh && res[k].health <= 0.f) {
                                    if (++it->dead_streak >= 10) {
                                        list.erase(it);
                                        break;
                                    }
                                } else {
                                    it->dead_streak = 0;
                                }
                                if (res[k].bones_ok) {

                                    const uint64_t rd_ms = res[k].read_ms ? res[k].read_ms : GetTickCount64();
                                    if (it->have_vel && (it->world_bones[10].X != 0.0 || it->world_bones[10].Y != 0.0 || it->world_bones[10].Z != 0.0)) {
                                        if (std::memcmp(&res[k].bones[10], &it->world_bones[10], sizeof(model::FVector)) == 0) ++s_same;
                                        else ++s_chg;
                                        const double dt = (double)(rd_ms >= it->bone_ms ? rd_ms - it->bone_ms : 0) / 1000.0;
                                        if (dt > 0.05 && dt < 2.0) {
                                            model::FVector inst =
                                                (res[k].bones[10] - it->world_bones[10]) * (1.0 / dt);


                                            {
                                                const double pdt = dt < 1.0 ? dt : 1.0;
                                                auto poff = it->bone_vel * pdt;
                                                const double pm = std::sqrt(poff.X * poff.X + poff.Y * poff.Y + poff.Z * poff.Z);
                                                if (pm > 80.0 && pm > 0.0) poff = poff * (80.0 / pm);
                                                auto derr = res[k].bones[10] - (it->world_bones[10] + poff);
                                                const double perr = std::sqrt(derr.X * derr.X + derr.Y * derr.Y + derr.Z * derr.Z);
                                                s_perr_sum += perr;
                                                if (perr > s_perr_max) s_perr_max = perr;
                                                ++s_perr_n;


                                                s_spd_sum += std::sqrt(it->bone_vel.X * it->bone_vel.X + it->bone_vel.Y * it->bone_vel.Y + it->bone_vel.Z * it->bone_vel.Z);
                                                auto ddv = inst - it->bone_vel;
                                                s_dvel_sum += std::sqrt(ddv.X * ddv.X + ddv.Y * ddv.Y + ddv.Z * ddv.Z);
                                            }
                                            const double sp = std::sqrt(inst.X * inst.X + inst.Y * inst.Y + inst.Z * inst.Z);



                                            model::FVector ni = inst;
                                            if (sp > 800.0 && sp < 2000.0) ni = inst * (800.0 / sp);
                                            if (sp < 2000.0) {


                                                const double old_sp = std::sqrt(it->bone_vel.X * it->bone_vel.X + it->bone_vel.Y * it->bone_vel.Y + it->bone_vel.Z * it->bone_vel.Z);



                                                if (old_sp > 60.0 && (sp < old_sp * 0.75 || sp < 40.0))
                                                    it->bone_vel = model::FVector{};
                                                else
                                                    it->bone_vel = it->bone_vel * 0.35 + ni * 0.65;
                                            }
                                            else
                                                it->bone_vel = model::FVector{};
                                        }
                                    } else {
                                        it->have_vel = true;
                                        it->bone_vel = model::FVector{};
                                    }
                                    if (it->bone_ms && rd_ms > it->bone_ms) {
                                        const float iv = (float)(rd_ms - it->bone_ms);
                                        if (iv > 5.f && iv < 1000.f)
                                            it->bone_interval = it->bone_interval * 0.7f + iv * 0.3f;
                                    }
                                    it->bone_ms = rd_ms;
                                    it->world_bones = res[k].bones;
                                    it->bones_synth = false;

                                    {
                                        const int bc = mesh_count(res[k].ptrs_updated ? res[k].mesh : jobs[k].mesh);
                                        if (bc > 0 && s_headdC.find(bc) == s_headdC.end()) {
                                            if (s_headdC.size() > 16) s_headdC.clear();
                                            if (auto rc = maxi::driver::g_driver->try_read<uintptr_t>(jobs[k].base_addr + memory_map::RootComponent); rc && *rc) {
                                                if (auto loc = maxi::driver::g_driver->try_read<model::FVector>(*rc + memory_map::RelativeLocation)) {
                                                    s_headdC[bc] = res[k].bones[0] - *loc;


                                                    model::FVector footp = res[k].bones[13];
                                                    if (res[k].bones[16].Z < footp.Z) footp = res[k].bones[16];
                                                    s_footdC[bc] = footp - *loc;
                                                }
                                            }
                                        }
                                    }
                                } else if (res[k].root_ok) {


                                    const int bc = mesh_count(jobs[k].mesh);
                                    model::FVector hd = model::FVector(0, 0, 175.0);
                                    model::FVector fd = model::FVector(0, 0, -90.0);
                                    if (bc > 0) {
                                        auto ith = s_headdC.find(bc);
                                        if (ith != s_headdC.end()) hd = ith->second;
                                        auto itf = s_footdC.find(bc);
                                        if (itf != s_footdC.end()) fd = itf->second;
                                    }
                                    const model::FVector head = res[k].root + hd;
                                    const model::FVector foot = res[k].root + fd;
                                    const uint64_t syn_ms = GetTickCount64();
                                    const model::FVector newp = head + (foot - head) * (10.0 / 16.0);
                                    if (it->have_vel && (it->world_bones[10].X != 0.0 || it->world_bones[10].Y != 0.0 || it->world_bones[10].Z != 0.0)) {
                                        const double dt = (double)(syn_ms >= it->bone_ms ? syn_ms - it->bone_ms : 0) / 1000.0;
                                        if (dt > 0.05 && dt < 2.0) {
                                            model::FVector inst = (newp - it->world_bones[10]) * (1.0 / dt);
                                            const double sp = std::sqrt(inst.X * inst.X + inst.Y * inst.Y + inst.Z * inst.Z);
                                            if (sp > 800.0 && sp < 2000.0) inst = inst * (800.0 / sp);
                                            if (sp < 2000.0) {
                                                const double old_sp = std::sqrt(it->bone_vel.X * it->bone_vel.X + it->bone_vel.Y * it->bone_vel.Y + it->bone_vel.Z * it->bone_vel.Z);
                                                if (old_sp > 100.0 && sp < old_sp * 0.75) it->bone_vel = model::FVector{};
                                                else it->bone_vel = it->bone_vel * 0.35 + inst * 0.65;
                                            } else it->bone_vel = model::FVector{};
                                        }
                                    } else it->have_vel = true;
                                    for (int bi = 0; bi < 17; ++bi) {
                                        const double t = (double)bi / 16.0;
                                        it->world_bones[(size_t)bi] = head + (foot - head) * t;
                                    }
                                    it->bone_ms = syn_ms;
                                    it->bones_synth = true;
                                    ++s_synth_runs;
                                }
                                if (refresh_aux && res[k].health > 0.f) {
                                    it->team_id = res[k].team;
                                    it->is_teammate = res[k].ally;
                                    it->visible = res[k].vis;
                                    it->aux_resolved = true;
                                    it->is_dormant = res[k].dorm;
                                    if (res[k].hasyaw) { it->facing_yaw = res[k].yaw; it->has_facing = true; }
                                    if (res[k].hide_eval) it->hidden = res[k].hide;
                                    if (!res[k].name.empty() && res[k].name != "??") {
                                        it->name = res[k].name;
                                        it->hidden = false;
                                    }
                                }
                                break;
                            }
                        }
                        m_actors_data_mutex.unlock();
                    } else { ++s_wbfail; }

                    {
                        static uint64_t s_job_dump_n = 0;
                        if (++s_job_dump_n % 100 == 1) {
                            for (size_t k = 0; k < njobs; ++k) {
                                LOG_WARNING(std::format("[JOB] {} hp={:.0f} dbg={}",
                                    jobs[k].dbg_name.empty() ? "??" : jobs[k].dbg_name,
                                    jobs[k].health, res[k].dbg ? res[k].dbg : "?"));
                            }
                        }
                    }
                    s_bones_rr += BONES_BATCH;
                }
            }
            catch (const std::exception& e) {
                LOG_ERROR(std::format("Exception in bones thread: {}", e.what()));
            }
            {
                static uint64_t s_cyc_n = 0;
                static double s_cyc_ms = 0.0;
                const auto cyc_end = std::chrono::steady_clock::now();
                const double cyc_ms = std::chrono::duration<double, std::milli>(cyc_end - cyc_start).count();
                s_cyc_ms += cyc_ms;
                if (cyc_ms > s_cyc_max) s_cyc_max = cyc_ms;

                {
                    using namespace maxi::driver;
                    const uint64_t c4 = g_cat_n[4].load() + g_cat_n[6].load() + g_cat_n[8].load();
                    const uint64_t cb = g_cat_bytes[4].load() + g_cat_bytes[6].load() + g_cat_bytes[8].load();
                    s_ph_r += (c4 - s_lc4); s_lc4 = c4;
                    s_bytes += (cb - s_lcb4); s_lcb4 = cb;
                }
                if (++s_cyc_n >= 50) {
                    const size_t nlist = m_actors_data.players_list.size();
                    const uint64_t btrans = maxi::driver::g_bone_transport_fail.exchange(0);
                    const uint64_t bgarb = maxi::driver::g_bone_garbage.exchange(0);
                    LOG_INFO(std::format("[BONES] avg_cycle={:.1f}ms max={:.0f}ms list={} (avg of 50 cycles) | jobs={} batt={} bok={} bbad={} sk_hp={} sk_dead={} sk_noba={} synthr={} readms={:.1f} | reads/cyc={} bytes/cyc={} wbfail={} jbfail={} trans={} garb={} same={} chg={} perr_avg={:.0f}u perr_max={:.0f}u spd={:.0f} dvel={:.0f}",
                        s_cyc_ms / (double)s_cyc_n, s_cyc_max, nlist,
                        s_jobs, s_batt, s_bok, s_bbad, s_sk_hp, s_sk_dead, s_sk_noba, s_synth_runs,
                        s_readms_n ? s_readms_sum / (double)s_readms_n : 0.0,
                        s_ph_r / s_cyc_n, s_bytes / s_cyc_n, s_wbfail, s_jbfail,
                        btrans, bgarb, s_same, s_chg,
                        s_perr_n ? s_perr_sum / (double)s_perr_n : 0.0, s_perr_max,
                        s_perr_n ? s_spd_sum / (double)s_perr_n : 0.0, s_perr_n ? s_dvel_sum / (double)s_perr_n : 0.0));
                    s_cyc_n = 0; s_cyc_ms = 0.0; s_cyc_max = 0.0;
                    s_ph_r = 0; s_bytes = 0; s_wbfail = 0; s_jbfail = 0;
                    s_batt = 0; s_bok = 0; s_bbad = 0; s_same = 0; s_chg = 0;
                    s_jobs = 0; s_sk_hp = 0; s_sk_dead = 0; s_sk_noba = 0; s_synth_runs = 0;
                    s_readms_sum = 0.0; s_readms_n = 0;
                    s_perr_sum = 0.0; s_perr_max = 0.0; s_perr_n = 0;
                    s_spd_sum = 0.0; s_dvel_sum = 0.0;
                }
            }
            Sleep(2);
        }
    }

    void CoreEngine::update_world_data() {
        WorldData new_data;
        new_data.last_update = std::chrono::steady_clock::now();

        try {
            auto world_addr = resolve_world_address();
            if (world_addr == 0) {
                static bool logged_once = false;
                if (!logged_once) {
                    LOG_WARNING("CoreEngine: resolve_world_address() returned 0!");
                    logged_once = true;
                }
                return;
            }

            new_data.world = model::UWorld(world_addr);
            if (m_last_world_addr != world_addr) {
                maxi::game::model::reset_global_fname_cache();
                m_last_world_addr = world_addr;
            }


            auto level_opt = new_data.world.TryGetPersistentLevel();
            new_data.level = level_opt.value_or(model::ULevel{});
            
            auto gi_opt = new_data.world.TryGetGameInstance();
            new_data.game_instance = gi_opt.value_or(model::UGameInstance{});

            static uint64_t diag_frame = 0;
            static bool logged_data_flow = false;
            if (!logged_data_flow || (++diag_frame % 60) == 0) {
                LOG_INFO(std::format("[DIAG ENGINE] WorldAddr: 0x{:X}, Level: 0x{:X}, GameInst: 0x{:X}",
                    world_addr, new_data.level.get_base_address(), new_data.game_instance.get_base_address()));
                logged_data_flow = true;
            }

            auto actors_opt = new_data.level.TryGetActors();
            new_data.actors = actors_opt.value_or(model::TArray<model::AActor>{});
            
            auto lp_opt = new_data.game_instance.is_valid() ? new_data.game_instance.TryGetLocalPlayer() : std::optional<model::ULocalPlayer>{};
            new_data.local_player = lp_opt.value_or(model::ULocalPlayer{});

            if (diag_frame % 60 == 0) {
                LOG_INFO(std::format("[DIAG ENGINE] LocalPlayer: 0x{:X}, Actors Count: {}, Actors.Data: 0x{:X}",
                    new_data.local_player.get_base_address(), new_data.actors.Num(),
                    (unsigned long long)new_data.actors.Data));
            }

            if (new_data.local_player.is_valid()) {
                auto pc_opt = new_data.local_player.TryGetPlayerController();
                new_data.player_controller = pc_opt.value_or(model::APlayerController{});
                if (new_data.player_controller.is_valid()) {
                    auto pcm_opt = new_data.player_controller.TryGetPlayerCameraManager();
                    new_data.player_camera_manager = pcm_opt.value_or(model::APlayerCameraManager{});
                    auto pawn_opt = new_data.player_controller.TryGetAcknowledgedPawn();
                    new_data.acknowledged_pawn = pawn_opt.value_or(model::AActor{});
                    m_local_pawn_addr.store(new_data.acknowledged_pawn.get_base_address());
                    if (new_data.acknowledged_pawn.is_valid()) {
                        static uintptr_t s_lpn_pawn = 0;
                        static std::string s_lpn;
                        const uintptr_t pw = new_data.acknowledged_pawn.get_base_address();
                        if (pw != s_lpn_pawn) {
                            s_lpn_pawn = pw;
                            s_lpn = GetAgentName(new_data.acknowledged_pawn.get_name_string());
                        }
                        new_data.local_player_name = s_lpn;
                    }
                }
                static bool s_pc_diag = false;
                if (!s_pc_diag) {
                    s_pc_diag = true;
                    const uintptr_t lp_base = new_data.local_player.get_base_address();
                    LOG_INFO(std::format("[PC DIAG] lp=0x{:X} pc=0x{:X} pcm=0x{:X} pawn=0x{:X}",
                        (unsigned long long)lp_base,
                        (unsigned long long)new_data.player_controller.get_base_address(),
                        (unsigned long long)new_data.player_camera_manager.get_base_address(),
                        (unsigned long long)new_data.acknowledged_pawn.get_base_address()));
                    if (new_data.player_camera_manager.is_valid()) {
                        const uintptr_t pcm_b = new_data.player_camera_manager.get_base_address();
                        auto loc = maxi::driver::g_driver->try_read<model::FVector>(pcm_b + memory_map::CameraCache + memory_map::ViewTargetPOV + memory_map::POVLocation).value_or(model::FVector{});
                        auto rot = maxi::driver::g_driver->try_read<model::FRotator>(pcm_b + memory_map::CameraCache + memory_map::ViewTargetPOV + memory_map::POVRotation).value_or(model::FRotator{});
                        auto fov = maxi::driver::g_driver->try_read<float>(pcm_b + memory_map::CameraCache + memory_map::ViewTargetPOV + memory_map::POVFOV).value_or(0.f);
                        LOG_INFO(std::format("[PC DIAG] cam loc=({:.1f},{:.1f},{:.1f}) rot=({:.1f},{:.1f},{:.1f}) fov={:.1f}",
                            loc.X, loc.Y, loc.Z, rot.Pitch, rot.Yaw, rot.Roll, fov));
                    }


                    {
                        const uintptr_t pc_cand = new_data.player_controller.get_base_address();
                        if (pc_cand > 0x10000000000ull && pc_cand < 0x40000000000ull) {
                            std::string pcscan;
                            for (uintptr_t off = 0x480; off <= 0x680; off += 8) {
                                auto v = maxi::driver::g_driver->try_read<uintptr_t>(pc_cand + off).value_or(0);
                                if (v > 0x10000000000ull && v < 0x40000000000ull) {
                                    char t[48];
                                    snprintf(t, sizeof(t), " +0x%llX=0x%llX", (unsigned long long)off, (unsigned long long)v);
                                    pcscan += t;
                                }
                            }
                            LOG_INFO(std::format("[PC DIAG] heap ptrs inside pc candidate (0x480-0x680):{}", pcscan));
                        }
                    }

                    if (lp_base) {
                        std::string scan;
                        for (uintptr_t off = 0; off <= 0x300; off += 8) {
                            auto v = maxi::driver::g_driver->try_read<uintptr_t>(lp_base + off).value_or(0);
                            if (v > 0x10000000000ull && v < 0x40000000000ull) {
                                char t[48];
                                snprintf(t, sizeof(t), " +0x%llX=0x%llX", (unsigned long long)off, (unsigned long long)v);
                                scan += t;
                            }
                        }
                        LOG_INFO(std::format("[PC DIAG] LP heap ptrs:{}", scan));

                        for (uintptr_t off : { (uintptr_t)0x20, (uintptr_t)0x48, (uintptr_t)0x80, (uintptr_t)0x88, (uintptr_t)0x90, (uintptr_t)0x98, (uintptr_t)0xA0 }) {
                            auto cand = maxi::driver::g_driver->try_read<uintptr_t>(lp_base + off).value_or(0);
                            if (cand < 0x10000000000ull || cand >= 0x40000000000ull) continue;
                            auto pawn  = maxi::driver::g_driver->try_read<uintptr_t>(cand + memory_map::AcknowledgedPawn).value_or(0);
                            auto cam   = maxi::driver::g_driver->try_read<uintptr_t>(cand + memory_map::PlayerCameraManager).value_or(0);
                            LOG_INFO(std::format("[PC DIAG] lp+0x{:X}=0x{:X} -> pawn@0x518=0x{:X} cam@0x528=0x{:X}",
                                (unsigned long long)off, (unsigned long long)cand,
                                (unsigned long long)pawn, (unsigned long long)cam));
                        }
                    }
                }
            }


            new_data.is_valid = (new_data.level.get_base_address() != 0 && new_data.actors.Num() > 0);

            new_data.is_valid = true;


            refresh_camera_cache(new_data.player_camera_manager);

            {
                std::lock_guard<std::mutex> lock(m_world_data_mutex);
                m_world_data = std::move(new_data);
            }


            if (m_world_callback && m_world_data.is_valid) {
                m_world_callback(m_world_data);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR(std::format("Exception updating world data: {}", e.what()));
        }
    }



    static constexpr bool kActorsPipelineEnabled = true;

    namespace {
        struct KeyProbe : public maxi::game::model::UObject {
            KeyProbe() : maxi::game::model::UObject(0) {}
            uint64_t get_key() { return static_cast<uint64_t>(DecryptNameKey()); }
        };
    }

    void CoreEngine::update_actors_data() {
        if (!kActorsPipelineEnabled) return;
        ActorsData updated_data;
        updated_data.last_update = std::chrono::steady_clock::now();

        try {
            WorldData world_data = get_world_data();
            if (world_data.is_valid || world_data.level.get_base_address() != 0) {
                const int32_t local_team_id =
                    world_data.acknowledged_pawn.is_valid()
                        ? world_data.acknowledged_pawn.TryGetTeamId().value_or(-1)
                        : -1;
                updated_data.local_team_id = local_team_id;

                constexpr int ACTOR_BUDGET = 128;
                static thread_local std::vector<ActorInfo> tmp_players;
                tmp_players.clear();


                struct SmartEntry { uintptr_t mesh = 0; uintptr_t bone_array = 0; int sweep = -1; };
                static thread_local std::unordered_map<uintptr_t, SmartEntry> s_smart;
                static thread_local int s_sweep = 0;

                static thread_local bool s_uid_enforce = true;
                static thread_local int s_empty_sweeps = 0;
                static thread_local int s_sweep_ok = 0;


                static thread_local std::unordered_map<uintptr_t, char> s_uidcache;
                static thread_local uintptr_t s_uidlevel = 0;

                static thread_local std::unordered_map<uintptr_t, int> s_nobad;
                static thread_local std::unordered_map<uintptr_t, int> s_bad;

                int c_invalid=0, c_self=0, c_nomesh=0, c_nobone=0, c_ok=0, c_hit=0, c_uid=0;
                auto _t0 = std::chrono::steady_clock::now();


                static thread_local uint64_t s_lc3 = 0, s_lcb3 = 0;




                {
                    const uintptr_t cur_level = world_data.level.get_base_address();
                    if (cur_level != 0 && cur_level != s_uidlevel) {
                        s_uidlevel = cur_level;
                        s_uidcache.clear();
                        s_smart.clear();
                        s_bad.clear();
                        s_nobad.clear();
                    }
                    if (s_uidcache.size() > 65536) s_uidcache.clear();
                }


                const int ARR_N = world_data.actors.Count;
                std::vector<uintptr_t> ptrs;
                if (ARR_N > 0 && world_data.actors.Data) {

                    const uintptr_t dptr = reinterpret_cast<uintptr_t>(world_data.actors.Data);
                    const int want = (std::min)(ARR_N, 2048);
                    ptrs.reserve((size_t)want);

                    auto whole = maxi::driver::g_driver->try_read_array_batch<uintptr_t>(dptr, want);
                    if (whole && (int)whole->size() == want) {
                        ptrs = std::move(*whole);
                    } else {
                        constexpr int CHUNK = 256;
                        for (int off = 0; off < want; off += CHUNK) {
                            const int n = (std::min)(CHUNK, want - off);
                            auto bulk = maxi::driver::g_driver->try_read_array_batch<uintptr_t>(
                                dptr + (uintptr_t)off * sizeof(uintptr_t), n);
                            if (!bulk || bulk->empty()) break;
                            ptrs.insert(ptrs.end(), bulk->begin(), bulk->end());
                            if ((int)bulk->size() < n) break;
                        }
                    }
                }
                const int NUM_ACT = (int)ptrs.size();
                const int KMAX = NUM_ACT;

                constexpr int NEW_BUDGET = 16;
                int new_reads = 0;
                const uintptr_t self_addr = world_data.acknowledged_pawn.is_valid()
                    ? world_data.acknowledged_pawn.get_base_address() : 0;
                if (NUM_ACT > 0) {
                    for (int k = 0; k < KMAX; ++k) {
                        const int i = k;

                        const uintptr_t base_addr = ptrs[(size_t)i];
                        if (!base_addr) { c_invalid++; continue; }
                        if (base_addr == self_addr) { c_self++; continue; }

                        {
                            auto itb = s_bad.find(base_addr);
                            if (itb != s_bad.end()) {
                                if (s_sweep - itb->second < 5) { c_nobone++; continue; }
                                s_bad.erase(itb);
                            }
                        }

                        uintptr_t mesh = 0, ba = 0;
                        auto itc = s_smart.find(base_addr);


                        if (s_uidcache.find(base_addr) == s_uidcache.end()) {
                            if (new_reads >= NEW_BUDGET) continue;
                            ++new_reads;
                        }


                        if (s_uid_enforce) {
                            char cached = 0;
                            auto itu = s_uidcache.find(base_addr);
                            if (itu == s_uidcache.end()) {
                                auto u = maxi::driver::g_driver->try_read<int32_t>(base_addr + memory_map::UniqueID);
                                if (!u) { c_uid++; continue; }
                                cached = (*u == 18743553) ? 1 : 2;



                                if (cached == 1) {
                                    auto dh = maxi::driver::g_driver->try_read<uintptr_t>(base_addr + memory_map::DamageHandler);
                                    if (!dh || !*dh) cached = 2;
                                    else {
                                        const uintptr_t mbase = maxi::driver::g_driver->get_base_address();
                                        const uintptr_t dhp = *dh;
                                        const uintptr_t upper = mbase ? mbase : (uintptr_t)0x7FFFFFFF0000ull;
                                        if (dhp < 0x10000 || dhp >= upper) cached = 2;
                                    }
                                }
                                s_uidcache[base_addr] = cached;
                            } else cached = itu->second;
                            if (cached != 1) { c_uid++; continue; }
                        }


                        if (itc != s_smart.end() && itc->second.mesh && itc->second.bone_array &&
                            ((s_sweep + (int)((base_addr >> 4) & 3)) % 3) != 0) {
                            mesh = itc->second.mesh; ba = itc->second.bone_array; c_hit++;
                        } else {


                            mesh = 0; ba = 0;
                        }

                        ActorInfo actor_info;
                        actor_info.actor = model::AActor(base_addr);
                        actor_info.mesh = mesh;
                        actor_info.bone_array = ba;
                        actor_info.name = "??";
                        actor_info.health = 100.f;
                        actor_info.visible = false;
                        actor_info.team_id = -1;
                        actor_info.is_teammate = false;
                        actor_info.aux_resolved = false;
                        actor_info.world_bones = {};
                        tmp_players.emplace_back(std::move(actor_info));
                        c_ok++;
                    }
                    s_sweep_ok += c_ok;
                    {
                        ++s_sweep;
                        for (auto it = s_smart.begin(); it != s_smart.end();) {
                            if (it->second.sweep < s_sweep - 3) it = s_smart.erase(it);
                            else ++it;
                        }
                        if (s_uid_enforce && s_sweep >= 1) {
                            if (s_sweep_ok == 0) {
                                if (++s_empty_sweeps >= 10) {
                                    s_uid_enforce = false;
                                    LOG_WARNING("[UID] 2 sweeps ok==0 - filter disabled");
                                }
                            } else s_empty_sweeps = 0;
                        }
                        s_sweep_ok = 0;
                    }
                }

                {
                    auto _t1 = std::chrono::steady_clock::now();
                    double ms = std::chrono::duration<double, std::milli>(_t1 - _t0).count();
                    static uint64_t pipe_log_n = 0;
                    static double max_ms = 0.0;
                    max_ms = (std::max)(max_ms, ms);

                    const uint64_t rc3b = maxi::driver::g_cat_n[3].load();
                    const uint64_t bc3b = maxi::driver::g_cat_bytes[3].load();
                    const uint64_t pr = rc3b - s_lc3; s_lc3 = rc3b;
                    const uint64_t pb = bc3b - s_lcb3; s_lcb3 = bc3b;
                    if ((++pipe_log_n % 30) == 1 || ms > 500.0) {
                        LOG_INFO(std::format("[ESP PIPE] total={} budget={} ok={} hit={} | drop: invalid={} self={} uid={} nomesh={} nobone={} | arrCount={} arrData=0x{:X} | pass={:.1f}ms max={:.0f}ms reads={} br={}",
                            NUM_ACT, ACTOR_BUDGET, c_ok, c_hit, c_invalid, c_self, c_uid, c_nomesh, c_nobone,
                            ARR_N, (unsigned long long)reinterpret_cast<uintptr_t>(world_data.actors.Data),
                            ms, max_ms, pr, pb));
                    }
                }

                static thread_local std::unordered_map<uintptr_t, int> s_miss;


                if (NUM_ACT > 0 && m_actors_data_mutex.try_lock()) {
                    auto& list = m_actors_data.players_list;
                    std::unordered_set<uintptr_t> seen_now;
                    seen_now.reserve(tmp_players.size());
                    for (auto& np : tmp_players) {
                        const uintptr_t ba = np.actor.get_base_address();
                        seen_now.insert(ba);
                        s_miss.erase(ba);
                        bool found = false;
                        for (auto& ex : list) {
                            if (ex.actor.get_base_address() == ba) {
                                ex.actor = np.actor;

                                if (np.mesh) ex.mesh = np.mesh;
                                if (np.bone_array && ex.bone_array != np.bone_array) {
                                    const uint64_t mnow = GetTickCount64();
                                    const bool fresh = ex.have_vel && mnow >= ex.bone_ms
                                        && (mnow - ex.bone_ms) <= 5000;
                                    if (fresh) {
                                        np.bone_array = ex.bone_array;
                                        s_smart[ba] = SmartEntry{ np.mesh, ex.bone_array, s_sweep };
                                    } else {
                                        ex.bone_array = np.bone_array;
                                    }
                                }
                                found = true;
                                break;
                            }
                        }
                        if (!found) list.push_back(std::move(np));
                    }

                    const int sweep_passes = NUM_ACT > 0 ? (NUM_ACT + KMAX - 1) / KMAX : 1;
                    const int MISS_LIMIT = sweep_passes + 2;
                    for (auto it = list.begin(); it != list.end(); ) {
                        const uintptr_t ba = it->actor.get_base_address();
                        if (seen_now.find(ba) != seen_now.end()) { ++it; continue; }

                        {
                            const uint64_t now = GetTickCount64();
                            const uint64_t last_b = (it->have_vel && now >= it->bone_ms) ? it->bone_ms : 0;
                            const uint64_t last_h = (it->health_ms != 0 && now >= it->health_ms) ? it->health_ms : 0;
                            const uint64_t last = last_b > last_h ? last_b : last_h;
                            if (last != 0 && now - last > 15000) {
                                s_miss.erase(ba);
                                it = list.erase(it);
                                continue;
                            }
                        }
                        if (++s_miss[ba] > MISS_LIMIT) { s_miss.erase(ba); it = list.erase(it); }
                        else ++it;
                    }
                    m_actors_data.local_team_id = updated_data.local_team_id;
                    m_actors_data.last_update = updated_data.last_update;
                    m_actors_data_mutex.unlock();
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR(std::format("Exception in actors data: {}", e.what()));
        }
    }
    void CoreEngine::update_skills_data() {
        SkillsData new_data;
        new_data.last_update = std::chrono::steady_clock::now();

        if (!var->config.visuals.skills)
            return;

        try {

            WorldData world_data = get_world_data();
            if (world_data.is_valid || world_data.level.get_base_address() != 0) {

                static thread_local std::unordered_map<uintptr_t, SkillInfo> skills_cache;

                static thread_local std::unordered_set<uintptr_t> seen_this_tick;
                seen_this_tick.clear();


                static thread_local std::vector<SkillInfo> tmp_skills;
                tmp_skills.clear();
                tmp_skills.reserve(static_cast<size_t>(world_data.actors.Num()));

                for (int i = 0; i < world_data.actors.Num(); i++) {
                    auto actor = world_data.actors[i];
                    if (!actor.is_valid()) continue;
                    if (actor.get_base_address() == world_data.acknowledged_pawn.get_base_address()) continue;

                    const auto base_addr = actor.get_base_address();
                    auto actor_name = actor.get_name_string();


                    if (!actor_name.empty() && actor_name == "TimedBomb_C") {
                        new_data.timed_bomb = actor;
                        continue;
                    }


                    if (auto it = skills_cache.find(base_addr); it != skills_cache.end()) {
                        it->second.skill = actor;
                        seen_this_tick.insert(base_addr);
                        continue;
                    }


                    auto name = GetSkillName(actor_name);
                    if (name.empty()) continue;

                    SkillInfo skill_info;
                    skill_info.skill = actor;
                    skill_info.name = name;

                    skills_cache.emplace(base_addr, std::move(skill_info));
                    seen_this_tick.insert(base_addr);
                }


                for (auto it = skills_cache.begin(); it != skills_cache.end(); ) {
                    if (seen_this_tick.find(it->first) == seen_this_tick.end()) {
                        it = skills_cache.erase(it);
                    } else {
                        ++it;
                    }
                }


                tmp_skills.reserve(tmp_skills.size() + skills_cache.size());
                for (const auto& kv : skills_cache) {
                    tmp_skills.emplace_back(kv.second);
                }
                new_data.skills_list.swap(tmp_skills);
            }


            {
                if (m_skills_data_mutex.try_lock()) {
                    m_skills_data = std::move(new_data);
                    m_skills_data_mutex.unlock();
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR(std::format("Exception updating actors data: {}", e.what()));
        }
    }

    bool CoreEngine::is_world_data_valid(const WorldData& data) const {
        return data.is_valid &&
               data.world.get_base_address() != 0 &&
               data.level.get_base_address() != 0;
    }

    static std::optional<std::vector<model::FVector>> ReadBonesFast(
        const model::AActor& actor, uintptr_t mesh, uintptr_t bone_array,
        const std::vector<int32_t>& bone_indices)
    {

        auto sanitize = [](std::vector<model::FVector>& v) -> bool {
            if (v.size() != 17) return false;
            auto bad = [](const model::FVector& p) {
                return !std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z) ||
                    std::abs(p.X) > 100000.0 || std::abs(p.Y) > 100000.0 || std::abs(p.Z) > 100000.0;
            };
            if (bad(v[0])) return false;
            const bool lfok = !bad(v[13]);
            const bool rfok = !bad(v[16]);
            if (!lfok && !rfok) return false;
            const model::FVector ref = v[0];
            for (auto& p : v) if (bad(p)) p = ref;
            if (!lfok) v[13] = v[16];
            if (!rfok) v[16] = v[13];
            return true;
        };
        auto spread_ok = [](const std::vector<model::FVector>& v) {
            if (v.size() < 2) return false;
            double mnx = 1e30, mny = 1e30, mnz = 1e30;
            double mxx = -1e30, mxy = -1e30, mxz = -1e30;
            for (const auto& p : v) {
                if (p.X < mnx) mnx = p.X; if (p.X > mxx) mxx = p.X;
                if (p.Y < mny) mny = p.Y; if (p.Y > mxy) mxy = p.Y;
                if (p.Z < mnz) mnz = p.Z; if (p.Z > mxz) mxz = p.Z;
            }
            const double dx = mxx - mnx, dy = mxy - mny, dz = mxz - mnz;
            return (dx * dx + dy * dy + dz * dz) > 80.0 * 80.0;
        };
        bool bulk_ok = false;
        static std::unordered_map<uintptr_t, ULONGLONG> s_retrycd;
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (attempt > 0) {
                const ULONGLONG rn = GetTickCount64();
                auto rit = s_retrycd.find(bone_array);
                if (rit != s_retrycd.end() && rn - rit->second < 2000) break;
                if (s_retrycd.size() > 256) s_retrycd.clear();
                s_retrycd[bone_array] = rn;
                Sleep(3);
            }
            if (auto fast = actor.TryGetBonesBatchStride(mesh, bone_array, bone_indices, memory_map::BoneStride)) {
                bulk_ok = true;
                std::vector<model::FVector> f = *fast;
                if (sanitize(f) && spread_ok(f)) return f;
                ++maxi::driver::g_bone_garbage;
                continue;
            }
            ++maxi::driver::g_bone_transport_fail;
            break;
        }

        return std::nullopt;
    }

    static void DumpBoneNamesOnce(uintptr_t mesh, int expected_num, uintptr_t bone_array, bool force = false) {
#ifndef _DEBUG
        (void)mesh; (void)expected_num; (void)bone_array; (void)force;
        return;
#else
        if (!mesh || expected_num < 16 || expected_num > 256 || !bone_array) return;
        static std::unordered_set<uintptr_t> s_dumped;
        if (!s_dumped.insert(bone_array).second) return;
        static ULONGLONG s_last_dump = 0;
        const ULONGLONG now = GetTickCount64();

        if (!force && now - s_last_dump < 120000) { s_dumped.erase(bone_array); return; }
        s_last_dump = now;

        auto drv = maxi::driver::g_driver;
        if (!drv) return;
        auto is_user_ptr = [](uint64_t p) { return p >= 0x10000ULL && (p >> 47) == 0; };
        auto read_u64 = [&](uintptr_t a, uint64_t& out) -> bool {
            auto o = drv->try_read<uint64_t>(a); if (!o) return false; out = *o; return true;
        };
        auto read_i32 = [&](uintptr_t a, int32_t& out) -> bool {
            auto o = drv->try_read<int32_t>(a); if (!o) return false; out = *o; return true;
        };

        uint64_t asset = 0;
        if (!read_u64(mesh + 0xA58, asset) || !is_user_ptr(asset)) {
            if (!read_u64(mesh + 0x708, asset) || !is_user_ptr(asset)) {
                LOG_WARNING(std::format("[BONEDUMP] ba={} asset not found", (unsigned long long)bone_array));
                return;
            }
        }

        struct Layout { uint32_t name_off, parent_off, stride; };
        static const Layout kLayouts[] = { {0,8,12},{0,8,16},{0,12,16},{0,12,20},{0,12,24},{0,16,20},{0,16,24} };
        auto looks_bone = [](const std::string& n) -> bool {
            if (n.size() < 3 || n.size() > 40) return false;
            std::string l = n;
            for (auto& c : l) if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
            static const char* toks[] = {"head","neck","spine","pelvis","shoulder","elbow","hand","hip","knee","foot","collar","clavicle","root","skeleton","thumb","finger","index","middle","ring","pinky","toe","thigh","weapon","jaw","brow","eye","mouth"};
            for (auto t : toks) if (l.find(t) != std::string::npos) return true;
            return false;
        };

        int best_score = -1;
        uint32_t best_off = 0; Layout best_L{}; int best_num = 0; uint64_t best_data = 0;
        for (uint32_t off = 0x80; off + 16 <= 0x800; off += 8) {
            uint64_t data = 0; int32_t num = 0, mx = 0;
            if (!read_u64(asset + off, data) || !is_user_ptr(data)) continue;
            if (!read_i32(asset + off + 8, num)) continue;
            if (!read_i32(asset + off + 12, mx)) continue;
            if (num < 16 || num > 256 || mx < num || mx > 1024) continue;
            int dd = num - expected_num; if (dd < 0) dd = -dd;
            if (dd > 3) continue;
            for (const auto& L : kLayouts) {
                const int sample = num < 12 ? num : 12;
                int structural = 0, parent0 = -99;
                bool bad = false;
                for (int i = 0; i < sample; ++i) {
                    int32_t p = 0;
                    if (!read_i32((uintptr_t)(data + (uint64_t)i * L.stride + L.parent_off), p)) { bad = true; break; }
                    if (i == 0) parent0 = p;
                    else if (p >= 0 && p < i) structural++;
                }
                if (bad || parent0 != -1) continue;
                if (structural < (sample - 1) * 9 / 10) continue;
                int hits = 0;
                std::string first;
                for (int i = 0; i < sample; ++i) {
                    int32_t ci = 0;
                    uint64_t e = data + (uint64_t)i * L.stride;
                    if (!read_i32((uintptr_t)e + L.name_off, ci) || ci == 0) continue;
                    std::string nm = model::UObject::resolve_fname_key(ci);
                    if (i == 0) first = nm;
                    if (looks_bone(nm)) hits++;
                }
                int score = hits * 2 + structural * 3 + 40;
                if (first.find("keleton") != std::string::npos || first.find("oot") != std::string::npos) score += 20;
                if (hits >= 3 && score > best_score) {
                    best_score = score; best_off = off; best_L = L; best_num = num; best_data = data;
                }
            }
        }
        if (!best_data) {
            LOG_WARNING(std::format("[BONEDUMP] ba={} count={} ref-skeleton not found", (unsigned long long)bone_array, expected_num));
            return;
        }
        LOG_WARNING(std::format("[BONEDUMP] ba={} count={} asset=0x{:X} array_off=0x{:X} noff={} poff={} stride={} score={}",
            (unsigned long long)bone_array, best_num, asset, best_off, best_L.name_off, best_L.parent_off, best_L.stride, best_score));
        const int dump_n = best_num < 160 ? best_num : 160;
        for (int i = 0; i < dump_n; ++i) {
            uint64_t e = best_data + (uint64_t)i * best_L.stride;
            int32_t ci = 0, parent = -999;
            read_i32((uintptr_t)e + best_L.name_off, ci);
            read_i32((uintptr_t)e + best_L.parent_off, parent);
            std::string nm = ci ? model::UObject::resolve_fname_key(ci) : "?";
            LOG_WARNING(std::format("[BONEDUMP] ba={} [{:3d}] parent={:4d} {}", (unsigned long long)bone_array, i, parent, nm));
        }
#endif
    }



    static std::optional<std::array<int, 17>> BuildNameBoneMap(uintptr_t mesh, int bone_count) {
        auto drv = maxi::driver::g_driver;
        if (!drv || !mesh) return std::nullopt;
        auto user_ptr = [](uint64_t p) { return p >= 0x10000ULL && (p >> 47) == 0; };
        uint64_t asset = 0;
        {
            auto a = drv->try_read<uint64_t>(mesh + 0xA58);
            if (a && user_ptr(*a)) asset = *a;
            else { auto b = drv->try_read<uint64_t>(mesh + 0x708); if (b && user_ptr(*b)) asset = *b; }
        }
        if (!asset) return std::nullopt;


        auto rd_chunk = [&](uintptr_t addr, uint8_t* dst, size_t n) {
            constexpr size_t CH = 0x100;
            for (size_t o = 0; o < n; o += CH) {
                const size_t c = (n - o < CH) ? (n - o) : CH;
                drv->read_memory(addr + o, dst + o, c);
            }
        };
        std::vector<uint8_t> atab(0xF80, 0);
        rd_chunk(asset + 0x80, atab.data(), atab.size());
        auto a64 = [&](uint32_t off) { uint64_t v = 0; std::memcpy(&v, atab.data() + (off - 0x80), 8); return v; };
        auto a32 = [&](uint32_t off) { int32_t v = 0; std::memcpy(&v, atab.data() + (off - 0x80), 4); return v; };
        struct L { uint32_t noff, poff, stride; };
        static const L layouts[] = { {0,8,12},{0,8,16},{0,12,16},{0,12,20},{0,12,24},{0,16,20},{0,16,24} };
        auto lower = [](std::string s) { for (auto& c : s) if (c >= 'A' && c <= 'Z') c = (char)(c + 32); return s; };
        std::string best_sample;

        for (uint32_t off = 0x80; off + 16 <= 0x1000; off += 8) {
            uint64_t data = a64(off);
            if (!user_ptr(data)) continue;
            int32_t num = a32(off + 8), mx = a32(off + 12);
            if (num < 16 || num > 512 || mx < num || mx > 1024) continue;


            int32_t dd = num - bone_count; if (dd < 0) dd = -dd;
            if (dd > 12) continue;
            L good{}; bool found = false;
            for (const auto& Lc : layouts) {
                const int sample = num < 12 ? num : 12;
                int32_t p0 = -999; int structural = 0; bool ok = true;
                for (int i = 0; i < sample; ++i) {
                    auto p = drv->try_read<int32_t>((uintptr_t)(data + (uint64_t)i * Lc.stride + Lc.poff));
                    if (!p) { ok = false; break; }
                    if (i == 0) p0 = *p; else if (*p >= 0 && *p < i) ++structural;
                }
                if (ok && p0 == -1 && structural >= (sample - 1) * 9 / 10) { good = Lc; found = true; break; }
            }
            if (!found) continue;
            std::vector<uint8_t> buf((size_t)num * good.stride, 0);
            rd_chunk(data, buf.data(), buf.size());
            int head=-1,neck=-1,chest=-1,lu=-1,lf=-1,lh=-1,ru=-1,rf=-1,rh=-1,stom=-1,pelv=-1,lt=-1,lk=-1,lft=-1,rt=-1,rk=-1,rft=-1;
            std::string names_sample;
            for (int i = 0; i < num; ++i) {
                int32_t ci = 0;
                std::memcpy(&ci, buf.data() + (size_t)i * good.stride + good.noff, sizeof(ci));
                if (!ci) continue;
                std::string nm = lower(model::UObject::resolve_fname_key(ci));
                if (i < 40 && names_sample.size() < 400) names_sample += std::format("{}:{},", i, nm.empty() ? "?" : nm);
                if (nm.empty() || nm == "none" || nm == "?") continue;
                if (head < 0 && nm == "head") head = i;
                else if (neck < 0 && nm == "neck") neck = i;
                else if (chest < 0 && (nm == "spine4" || nm == "spine3")) chest = i;
                else if (stom < 0 && nm == "spine1") stom = i;
                else if (pelv < 0 && nm == "pelvis") pelv = i;
                else if (lu < 0 && (nm == "l_shoulder" || nm == "l_upperarm")) lu = i;
                else if (lf < 0 && (nm == "l_elbow" || nm == "l_lowerarm")) lf = i;
                else if (lh < 0 && nm == "l_hand") lh = i;
                else if (ru < 0 && (nm == "r_shoulder" || nm == "r_upperarm")) ru = i;
                else if (rf < 0 && (nm == "r_elbow" || nm == "r_lowerarm")) rf = i;
                else if (rh < 0 && nm == "r_hand") rh = i;
                else if (lt < 0 && nm == "l_hip") lt = i;
                else if (lk < 0 && nm == "l_knee") lk = i;
                else if (lft < 0 && nm == "l_foot") lft = i;
                else if (rt < 0 && nm == "r_hip") rt = i;
                else if (rk < 0 && nm == "r_knee") rk = i;
                else if (rft < 0 && nm == "r_foot") rft = i;
            }
            if (head < 0 || neck < 0 || chest < 0 || pelv < 0 || lu < 0 || lf < 0 || lh < 0 ||
                ru < 0 || rf < 0 || rh < 0 || lt < 0 || lk < 0 || lft < 0 || rt < 0 || rk < 0 || rft < 0) {
                if (best_sample.empty()) best_sample = std::format("off=0x{:X} {}", off, names_sample);
                continue;
            }
            if (stom < 0) stom = chest;
            LOG_WARNING(std::format("[NAMEMAP] bc={} off=0x{:X} h={} n={} c={} p={} | lArm={}/{}/{} rArm={}/{}/{} | lLeg={}/{}/{} rLeg={}/{}/{}",
                bone_count, off, head, neck, chest, pelv, lu, lf, lh, ru, rf, rh, lt, lk, lft, rt, rk, rft));
            return std::array<int, 17>{ head, neck, chest, lu, lf, lh, ru, rf, rh, stom, pelv, lt, lk, lft, rt, rk, rft };
        }
        {
            static std::unordered_set<int> s_nmdiag;
            if (s_nmdiag.insert(bone_count).second)
                LOG_WARNING(std::format("[NAMEMAP] bc={} COULD NOT BUILD | sample:{}", bone_count, best_sample.empty() ? "(no candidates)" : best_sample));
        }
        return std::nullopt;
    }

    std::optional<std::array<model::FVector, 17>> CoreEngine::GetActorBoneWorldPositions(model::AActor actor, uintptr_t mesh, uintptr_t bone_array)
    {
        auto bone_count = actor.BoneCount(mesh);




        if (bone_count < 16 || bone_count > 512) {
            static std::unordered_set<uintptr_t> s_glogged;
            if (s_glogged.insert(bone_array).second)
                LOG_WARNING(std::format("[BONECOUNT] ba=0x{:X} count={} rejected",
                    (unsigned long long)bone_array, bone_count));
            return std::nullopt;
        }

        struct BoneMap {
            int head, neck, chest;
            int lUpperArm, lForeArm, lHand;
            int rUpperArm, rForeArm, rHand;
            int stomach, pelvis;
            int lThigh, lKnee, lFoot;
            int rThigh, rKnee, rFoot;
        };








        static const std::unordered_map<int, BoneMap> boneMaps = {
            { 106, {8,7,6, 25,26,27, 51,52,53, 3,76, 79,80,82, 86,87,89} },


            { 114, {8,7,6, 25,26,27, 59,60,61, 3,84, 87,88,90, 94,95,97} },



            { 119, {8,7,6, 21,22,23, 49,50,51, 3,89, 92,93,95, 99,100,102} },




            { 123, {8,7,6, 37,38,39, 63,64,65, 3,88, 91,92,94, 98,99,101} }
        };


        if (bone_count == 103) {
            auto anatomical = [](const std::array<model::FVector,17>& w) {
                const double hf = w[0].Z - w[13].Z;
                const double hn = w[0].Z - w[1].Z;
                return hf > 100.0 && hf < 3000.0 && hn > 1.0;
            };
            static const std::array<int,17> sage103 = {8,7,6, 25,26,27, 50,51,52, 3,74, 77,78,80, 84,85,87};
            static const std::array<int,17> bot103 = {8,7,6, 11,12,13, 36,37,38, 3,60, 63,64,66, 77,78,80};
            const std::array<int,17>* sets[2] = { &sage103, &bot103 };
            for (int s = 0; s < 2; ++s) {
                std::vector<int32_t> bi(sets[s]->begin(), sets[s]->end());
                if (auto p = ReadBonesFast(actor, mesh, bone_array, bi)) {
                    if (p->size() == 17) {
                        std::array<model::FVector,17> w{};
                        for (size_t i = 0; i < 17; ++i) w[i] = (*p)[i];
                        if (anatomical(w)) return w;
                    }
                }
            }
        }

        auto it = boneMaps.find(bone_count);
        if (it != boneMaps.end()) {
#ifdef _DEBUG
            if (bone_count == 114) {
                static std::unordered_set<uintptr_t> s_114scan;
                if (s_114scan.insert(bone_array).second) {
                    auto ctw = maxi::driver::g_driver->try_read<model::FTransform>(mesh + memory_map::ComponentToWorld);
                    if (ctw) {
                        const size_t tot = (size_t)bone_count * memory_map::BoneStride;
                        std::vector<std::uint8_t> bf(tot);
                        ++maxi::driver::g_read_calls;
                        if (maxi::driver::g_driver->read_memory(bone_array, bf.data(), tot)) {
                            auto wp = [&](int idx) {
                                model::FTransform t{};
                                std::memcpy(&t, bf.data() + (size_t)idx * memory_map::BoneStride, sizeof(t));
                                model::FMatrix m = t.ToMatrixWithScale() * ctw->ToMatrixWithScale();
                                return model::FVector(m.WPlane.X, m.WPlane.Y, m.WPlane.Z);
                            };
                            auto L = [&](int i) { auto v = wp(i); return std::format("{}:{:.0f},{:.0f}", i, v.Z, v.X); };
                            LOG_WARNING(std::format("[LEG114] head8={} neck7={} | Lthigh87={} Lknee88={} Lfoot90={} | Rthigh94={} Rknee95={} Rfoot97={}",
                                L(8), L(7), L(87), L(88), L(90), L(94), L(95), L(97)));
                        }
                    }
                }
            }
#endif
            const BoneMap& map = it->second;
            std::array<int, 17> boneIdx = {
                map.head, map.neck, map.chest,
                map.lUpperArm, map.lForeArm, map.lHand,
                map.rUpperArm, map.rForeArm, map.rHand,
                map.stomach, map.pelvis,
                map.lThigh, map.lKnee, map.lFoot,
                map.rThigh, map.rKnee, map.rFoot
            };
            std::vector<int32_t> bone_indices(boneIdx.begin(), boneIdx.end());
            auto pos_opt = ReadBonesFast(actor, mesh, bone_array, bone_indices);
            if (!pos_opt || pos_opt->size() != 17) {
                DumpBoneNamesOnce(mesh, bone_count, bone_array, false);
                return std::nullopt;
            }
            {
                const auto& p = *pos_opt;
                const double hf = p[0].Z - p[13].Z;
                const double hn = p[0].Z - p[1].Z;
                if (!(hf > 80.0 && hf < 3000.0 && hn > 1.0)) {
                    DumpBoneNamesOnce(mesh, bone_count, bone_array, false);

#ifdef _DEBUG
                    {
                        static std::unordered_set<uintptr_t> s_stp;
                        if (s_stp.insert(bone_array).second) {
                            bool st_hit = false;
                            if (auto ctw = maxi::driver::g_driver->try_read<model::FTransform>(mesh + memory_map::ComponentToWorld)) {
                                const size_t big = (size_t)bone_count * 0xA0u;
                                std::vector<std::uint8_t> bb(big);
                                ++maxi::driver::g_read_calls;
                                maxi::driver::g_read_bytes += big;
                                if (maxi::driver::g_driver->read_memory(bone_array, bb.data(), big)) {
                                    for (uint32_t st : { 0x50u, 0x40u, 0x70u, 0x30u, 0x80u, 0xA0u }) {
                                        bool okk = true;
                                        model::FVector pts[17];
                                        for (int ii = 0; ii < 17; ++ii) {
                                            const size_t off = (size_t)bone_indices[(size_t)ii] * st;
                                            if (off + sizeof(model::FTransform) > big) { okk = false; break; }
                                            model::FTransform t{};
                                            std::memcpy(&t, bb.data() + off, sizeof(t));
                                            model::FMatrix m = t.ToMatrixWithScale() * ctw->ToMatrixWithScale();
                                            pts[ii] = model::FVector(m.WPlane.X, m.WPlane.Y, m.WPlane.Z);
                                            if (!std::isfinite(pts[ii].X) || !std::isfinite(pts[ii].Y) || !std::isfinite(pts[ii].Z) ||
                                                std::abs(pts[ii].X) > 100000.0 || std::abs(pts[ii].Y) > 100000.0 || std::abs(pts[ii].Z) > 100000.0) { okk = false; break; }
                                        }
                                        if (okk) {
                                            const double shf = pts[0].Z - pts[13].Z;
                                            const double shn = pts[0].Z - pts[1].Z;
                                            if (shf > 80.0 && shf < 3000.0 && shn > 1.0) {
                                                LOG_WARNING(std::format("[STRIDE] ba=0x{:X} stride=0x{:X} TUTTU (hf={:.0f})",
                                                    (unsigned long long)bone_array, st, shf));
                                                st_hit = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if (!st_hit)
                                LOG_WARNING(std::format("[STRIDE] ba=0x{:X} no match (not stride, memory garbage)",
                                    (unsigned long long)bone_array));
                        }
                    }
#endif
                    return std::nullopt;
                }
            }
            std::array<model::FVector, 17> world{};
            for (size_t i = 0; i < 17; ++i)
                world[i] = (*pos_opt)[i];
            return world;
        }

        {
            static std::unordered_map<int, std::array<int, 17>> s_namemap;
            static std::unordered_map<int, ULONGLONG> s_namemap_fail;
            auto nmit = s_namemap.find(bone_count);
            if (nmit == s_namemap.end()) {
                auto nf = s_namemap_fail.find(bone_count);

                if (nf == s_namemap_fail.end() || GetTickCount64() - nf->second >= 60000) {
                    if (auto built = BuildNameBoneMap(mesh, bone_count)) {
                        s_namemap[bone_count] = *built;
                        nmit = s_namemap.find(bone_count);
                    } else {
                        s_namemap_fail[bone_count] = GetTickCount64();
                        static std::unordered_set<int> s_nmlog;
                        if (s_nmlog.insert(bone_count).second) {
                            const bool bod = model::AActor::MeshHasBodyBone(mesh);
                            std::string cs;
                            for (uintptr_t mo : { memory_map::Mesh, memory_map::MeshCharacter, memory_map::Mesh1P, memory_map::MeshOverlay1P, (uintptr_t)0xF60, (uintptr_t)0xF70, (uintptr_t)0x0F28 }) {
                                auto mv = maxi::driver::g_driver->try_read<uintptr_t>(actor.get_base_address() + mo).value_or(0);
                                int bcv = mv ? actor.BoneCount(mv) : 0;
                                bool bd = mv && model::AActor::MeshHasBodyBone(mv);
                                cs += std::format(" +0x{:X}=0x{:X}(bc={},body={})", (unsigned long long)mo, (unsigned long long)mv, bcv, bd ? 1 : 0);
                            }
                            LOG_WARNING(std::format("[NAMEMAP] bc={} COULD NOT BUILD mesh=0x{:X} body={} |{}",
                                bone_count, (unsigned long long)mesh, bod ? 1 : 0, cs));
                        }
                    }
                }
            }
            if (nmit != s_namemap.end()) {
                std::vector<int32_t> bone_indices(nmit->second.begin(), nmit->second.end());
                auto pos_opt = ReadBonesFast(actor, mesh, bone_array, bone_indices);
                if (pos_opt && pos_opt->size() == 17) {
                    const auto& p = *pos_opt;
                    const double hf = p[0].Z - p[13].Z;
                    const double hn = p[0].Z - p[1].Z;
                    if (hf > 80.0 && hf < 3000.0 && hn > 1.0) {
                        std::array<model::FVector, 17> world{};
                        for (size_t i = 0; i < 17; ++i) world[i] = p[i];
                        return world;
                    }
                }
            }
        }


        static std::unordered_map<uintptr_t, BoneMap> s_discovered;

        static std::unordered_map<uintptr_t, ULONGLONG> s_disc_neg;
        auto dit = s_discovered.find(bone_array);
        if (dit == s_discovered.end()) {
            {
                static ULONGLONG s_last_disc = 0;
                const ULONGLONG now = GetTickCount64();


                if (now - s_last_disc < 3000) return std::nullopt;
                s_last_disc = now;
                auto itn = s_disc_neg.find(bone_array);
                if (itn != s_disc_neg.end()) {
                    if (now - itn->second < 120000) return std::nullopt;
                    s_disc_neg.erase(itn);
                }
            }
            DumpBoneNamesOnce(mesh, bone_count, bone_array, false);
            std::vector<model::FVector> all(bone_count);
            bool ok = true;
            {
                auto ctw_opt = maxi::driver::g_driver->try_read<model::FTransform>(mesh + memory_map::ComponentToWorld);
                if (!ctw_opt) return std::nullopt;
                const uintptr_t stride = memory_map::BoneStride;
                const size_t total = static_cast<size_t>(bone_count) * stride;
                std::vector<std::uint8_t> buf(total);
                ++maxi::driver::g_read_calls;
                maxi::driver::g_read_bytes += total;
                if (!maxi::driver::g_driver->read_memory(bone_array, buf.data(), total)) return std::nullopt;
                for (int i = 0; i < bone_count; ++i) {
                    model::FTransform t{};
                    std::memcpy(&t, buf.data() + static_cast<size_t>(i) * stride, sizeof(t));
                    model::FMatrix m = t.ToMatrixWithScale() * ctw_opt->ToMatrixWithScale();
                    all[i] = model::FVector(m.WPlane.X, m.WPlane.Y, m.WPlane.Z);
                    if (!std::isfinite(all[i].X) || !std::isfinite(all[i].Y) || !std::isfinite(all[i].Z) ||
                        std::abs(all[i].X) > 100000.0 || std::abs(all[i].Y) > 100000.0 || std::abs(all[i].Z) > 100000.0)
                        all[i].Z = std::numeric_limits<double>::quiet_NaN();
                }
                int sane_cnt = 0;
                for (int i = 0; i < bone_count; ++i)
                    if (!std::isnan(all[i].Z)) ++sane_cnt;
                if (sane_cnt < bone_count / 3) ok = false;
            }
            if (!ok) {
                s_disc_neg[bone_array] = GetTickCount64();
                static std::unordered_set<uintptr_t> s_fail2;
                if (s_fail2.insert(bone_array).second)
                    LOG_WARNING(std::format("[BONEMAP] ba={} bone_count={} bulk garbage - discovery skipped",
                        (unsigned long long)bone_array, bone_count));
                return std::nullopt;
            }
            if (ok) {
#ifdef _DEBUG
                {
                    static std::unordered_set<uintptr_t> s_scanned;
                    if (s_scanned.insert(bone_array).second) {
                        double zmn = 1e30, zmx = -1e30;
                        int imn = 0, imx = 0;
                        for (int i = 0; i < bone_count; ++i) {
                            if (all[i].Z < zmn) { zmn = all[i].Z; imn = i; }
                            if (all[i].Z > zmx) { zmx = all[i].Z; imx = i; }
                        }
                        std::string zlist;
                        for (int i = 0; i < bone_count; ++i)
                            zlist += std::format(" {}:{:.0f}", i, all[i].Z);
                        LOG_INFO(std::format("[BONE SCAN] ba={} count={} minZ={:.0f}@{} maxZ={:.0f}@{} |{}",
                            (unsigned long long)bone_array, bone_count, zmn, imn, zmx, imx, zlist));
                    }
                }
#endif



                std::vector<double> zvals;
                zvals.reserve(bone_count);
                for (int i = 0; i < bone_count; ++i) zvals.push_back(all[i].Z);
                std::sort(zvals.begin(), zvals.end());
                double z_ref = zvals[zvals.size() / 2];
                constexpr double Z_BAND = 400.0;
                for (int i = 0; i < bone_count; ++i)
                    if (std::abs(all[i].Z - z_ref) > Z_BAND)
                        all[i].Z = std::numeric_limits<double>::quiet_NaN();

                double z_min = 1e30, z_max = -1e30;
                int i_min = 0, i_max = 0;
                for (int i = 0; i < bone_count; ++i) {
                    if (std::isnan(all[i].Z)) continue;
                    if (all[i].Z < z_min) { z_min = all[i].Z; i_min = i; }
                    if (all[i].Z > z_max) { z_max = all[i].Z; i_max = i; }
                }
                const double H = z_max - z_min;
                if (H > 20.0 && H < 2000.0) {                    BoneMap bm{};
                    std::vector<char> used(bone_count, 0);
                    auto is_nan = [&](int i) { return std::isnan(all[i].Z); };

                    std::vector<int> byZ;
                    byZ.reserve(bone_count);
                    for (int i = 0; i < bone_count; ++i) if (!is_nan(i)) byZ.push_back(i);
                    std::sort(byZ.begin(), byZ.end(), [&](int a, int b) { return all[a].Z > all[b].Z; });
                    auto pick_rank = [&](int rank) -> int {
                        int c = 0;
                        for (int idx : byZ) { if (!used[idx] && c++ == rank) return idx; }
                        return -1;
                    };
                    bm.head = pick_rank(0);
                    bm.neck = pick_rank(1);
                    bm.chest = pick_rank(2);
                    if (bm.head < 0 || bm.neck < 0 || bm.chest < 0) return {};
                    used[bm.head] = used[bm.neck] = used[bm.chest] = 1;
                    auto pick_near_z = [&](double zt) -> int {
                        int best = -1; double bs = 1e30;
                        for (int i = 0; i < bone_count; ++i) {
                            if (used[i] || is_nan(i)) continue;
                            const double d = std::abs(all[i].Z - zt);
                            if (d < bs) { bs = d; best = i; }
                        }
                        return best;
                    };
                    bm.pelvis = pick_near_z(z_min + 0.5 * H);
                    bm.stomach = pick_near_z(z_min + 0.65 * H);
                    if (bm.pelvis < 0 || bm.stomach < 0) return {};
                    used[bm.pelvis] = 1; used[bm.stomach] = 1;


                    auto pick_low = [&](int skip) -> int {
                        int best = -1; double bs = 1e30;
                        for (int i = 0; i < bone_count; ++i) {
                            if (used[i] || is_nan(i) || i == skip) continue;
                            const double d = std::abs(all[i].Z - z_min);
                            if (d < bs) { bs = d; best = i; }
                        }
                        return best;
                    };
                    int f1 = pick_low(-1);
                    int f2 = (f1 >= 0) ? pick_low(f1) : -1;
                    if (f1 < 0 || f2 < 0) return {};
                    used[f1] = 1; used[f2] = 1;
                    bm.lFoot = (all[f1].X < all[f2].X) ? f1 : f2;
                    bm.rFoot = (all[f1].X < all[f2].X) ? f2 : f1;
                    auto pick_limb = [&](double zt, double xref, int ex1, int ex2) -> int {
                        int best = -1; double bs = 1e30;
                        for (int i = 0; i < bone_count; ++i) {
                            if (used[i] || is_nan(i) || i == ex1 || i == ex2) continue;
                            const double d = std::abs(all[i].Z - zt) + std::abs(all[i].X - xref) * 0.3;
                            if (d < bs) { bs = d; best = i; }
                        }
                        return best;
                    };
                    bm.lKnee = pick_limb(z_min + 0.25 * H, all[bm.lFoot].X, bm.lFoot, bm.rFoot);
                    if (bm.lKnee < 0) return {};
                    used[bm.lKnee] = 1;
                    bm.rKnee = pick_limb(z_min + 0.25 * H, all[bm.rFoot].X, bm.lFoot, bm.rFoot);
                    if (bm.rKnee < 0) return {};
                    used[bm.rKnee] = 1;
                    bm.lThigh = pick_limb(z_min + 0.38 * H, all[bm.lFoot].X, bm.lKnee, bm.rKnee);
                    if (bm.lThigh < 0) return {};
                    used[bm.lThigh] = 1;
                    bm.rThigh = pick_limb(z_min + 0.38 * H, all[bm.rFoot].X, bm.lKnee, bm.rKnee);
                    if (bm.rThigh < 0) return {};
                    used[bm.rThigh] = 1;
                    const double neck_z = all[bm.neck].Z;
                    const double cx = all[bm.head].X;
                    int lsh = -1, rsh = -1; double ls = 1e30, rs = 1e30;
                    for (int i = 0; i < bone_count; ++i) {
                        if (used[i] || is_nan(i)) continue;
                        const double dz = std::abs(all[i].Z - neck_z);
                        const double dx = all[i].X - cx;
                        if (dx < 0) { const double d = dz - dx; if (d < ls) { ls = d; lsh = i; } }
                        else        { const double d = dz + dx; if (d < rs) { rs = d; rsh = i; } }
                    }
                    if (lsh < 0 || rsh < 0 || lsh == rsh) return {};
                    used[lsh] = 1; used[rsh] = 1;
                    bm.lUpperArm = (all[lsh].X < all[rsh].X) ? lsh : rsh;
                    bm.rUpperArm = (all[lsh].X < all[rsh].X) ? rsh : lsh;
                    auto pick_hand = [&](int side) -> int {
                        int best = -1; double bs = 1e30;
                        for (int i = 0; i < bone_count; ++i) {
                            if (used[i] || is_nan(i)) continue;
                            const double dz = std::abs(all[i].Z - (z_min + 0.45 * H));
                            const double dx = all[i].X - cx;
                            if ((side < 0 && dx >= 0) || (side > 0 && dx <= 0)) continue;
                            const double d = dz - std::abs(dx);
                            if (d < bs) { bs = d; best = i; }
                        }
                        return best;
                    };
                    int lh = pick_hand(-1), rh = pick_hand(1);
                    if (lh < 0 || rh < 0) return {};
                    used[lh] = 1; used[rh] = 1;
                    bm.lHand = lh; bm.rHand = rh;

                    auto pick_elbow = [&](int sh, int hand) -> int {
                        int best = -1; double bs = 1e30;
                        const double mx = (all[sh].X + all[hand].X) * 0.5;
                        const double mz = (all[sh].Z + all[hand].Z) * 0.5;
                        for (int i = 0; i < bone_count; ++i) {
                            if (used[i] || is_nan(i)) continue;
                            const double d = std::abs(all[i].Z - mz) + std::abs(all[i].X - mx) * 0.5;
                            if (d < bs) { bs = d; best = i; }
                        }
                        return best;
                    };
                    bm.lForeArm = pick_elbow(bm.lUpperArm, bm.lHand);
                    bm.rForeArm = pick_elbow(bm.rUpperArm, bm.rHand);
                    if (bm.lForeArm < 0 || bm.rForeArm < 0) return {};
                    used[bm.lForeArm] = 1; used[bm.rForeArm] = 1;


                    {
                        const int all17[17] = { bm.head, bm.neck, bm.chest,
                            bm.lUpperArm, bm.lForeArm, bm.lHand,
                            bm.rUpperArm, bm.rForeArm, bm.rHand,
                            bm.stomach, bm.pelvis,
                            bm.lThigh, bm.lKnee, bm.lFoot,
                            bm.rThigh, bm.rKnee, bm.rFoot };
                        bool seen[512] = {};
                        bool okv = true;
                        for (int v : all17) {
                            if (v < 0 || v >= bone_count || seen[v]) { okv = false; break; }
                            seen[v] = true;
                        }
                        if (okv) {
                            const double hf = all[bm.head].Z - all[bm.lFoot].Z;
                            const double hn = all[bm.head].Z - all[bm.neck].Z;
                            if (!(hf > 80.0 && hf < 3000.0 && hn > 1.0)) okv = false;
                        }
                        if (!okv) {
                            int distinct = 0;
                            {
                                bool seen2[512] = {};
                                for (int v : all17) {
                                    if (v >= 0 && v < bone_count && !seen2[v]) { seen2[v] = true; ++distinct; }
                                }
                            }
                            const double hf = (bm.head >= 0 && bm.lFoot >= 0) ? (all[bm.head].Z - all[bm.lFoot].Z) : -1.0;
                            const double hn = (bm.head >= 0 && bm.neck >= 0) ? (all[bm.head].Z - all[bm.neck].Z) : -1.0;
                            static std::unordered_set<uintptr_t> s_badlogged;
                            if (s_badlogged.insert(bone_array).second)
                                LOG_WARNING(std::format("[BONEMAP] ba={} bone_count={} validation failed (distinct={}/17 hf={:.0f} hn={:.0f}) - not cached",
                                    (unsigned long long)bone_array, bone_count, distinct, hf, hn));
                            s_disc_neg[bone_array] = GetTickCount64();
                            return {};
                        }
                    }

                    s_discovered[bone_array] = bm;
                    s_disc_neg.erase(bone_array);
                    static std::unordered_set<uintptr_t> s_logged;
                    if (s_logged.insert(bone_array).second)
                        LOG_WARNING(std::format("[BONEMAP] ba={} bone_count={} auto-truncated: h={} n={} c={} p={} s={} la={} le={} lh={} ra={} re={} rh={} lt={} lk={} lf={} rt={} rk={} rf={}",
                            (unsigned long long)bone_array, bone_count,
                            bm.head, bm.neck, bm.chest, bm.pelvis, bm.stomach,
                            bm.lUpperArm, bm.lForeArm, bm.lHand,
                            bm.rUpperArm, bm.rForeArm, bm.rHand,
                            bm.lThigh, bm.lKnee, bm.lFoot, bm.rThigh, bm.rKnee, bm.rFoot));
                    dit = s_discovered.find(bone_array);
                }
            }
            if (dit == s_discovered.end()) {
                static std::unordered_set<uintptr_t> s_fail;
                if (s_fail.insert(bone_array).second)
                    LOG_WARNING(std::format("[BONEMAP] ba={} bone_count={} discovery failed",
                        (unsigned long long)bone_array, bone_count));
                s_disc_neg[bone_array] = GetTickCount64();
                return {};
            }
        }

        const BoneMap& map = dit->second;
        std::array<int, 17> boneIdx = {
            map.head, map.neck, map.chest,
            map.lUpperArm, map.lForeArm, map.lHand,
            map.rUpperArm, map.rForeArm, map.rHand,
            map.stomach, map.pelvis,
            map.lThigh, map.lKnee, map.lFoot,
            map.rThigh, map.rKnee, map.rFoot
        };
        std::vector<int32_t> bone_indices(boneIdx.begin(), boneIdx.end());
        auto pos_opt = ReadBonesFast(actor, mesh, bone_array, bone_indices);
        if (!pos_opt || pos_opt->size() != 17) return std::nullopt;
        {
            const auto& p = *pos_opt;
            const double hf = p[0].Z - p[13].Z;
            const double hn = p[0].Z - p[1].Z;
            if (!(hf > 80.0 && hf < 3000.0 && hn > 1.0)) {
                s_discovered.erase(bone_array);
                s_disc_neg[bone_array] = GetTickCount64();
                return std::nullopt;
            }
        }
        std::array<model::FVector, 17> world{};
        for (size_t i = 0; i < 17; ++i)
            world[i] = (*pos_opt)[i];
        return world;
    }

    std::optional<std::pair<std::array<model::FVector2D, 17>, std::array<std::array<model::FVector2D, 2>, 16>>> CoreEngine::SetupActorSkeleton(model::AActor actor, uintptr_t mesh, uintptr_t bone_array)
    {
        auto world_opt = GetActorBoneWorldPositions(actor, mesh, bone_array);
        if (!world_opt) return std::nullopt;
        auto world = *world_opt;

        std::array<model::FVector2D, 17> bones{};
        std::array<std::array<model::FVector2D, 2>, 16> skeleton{};

        for (size_t i = 0; i < world.size(); ++i) {
            bones[i] = ProjectWorldToScreen(world[i]);
        }

        static const std::array<std::pair<int, int>, 16> skeletonLinks = { {
            {0,1}, {1,2}, {2,3}, {3,4}, {4,5},
            {2,6}, {6,7}, {7,8},
            {2,9}, {9,10},
            {10,11}, {11,12}, {12,13},
            {10,14}, {14,15}, {15,16}
        } };

        for (size_t i = 0; i < skeletonLinks.size(); ++i) {
            skeleton[i][0] = bones[skeletonLinks[i].first];
            skeleton[i][1] = bones[skeletonLinks[i].second];
        }

        return std::make_pair(bones, skeleton);
    }

	void CoreEngine::refresh_camera_cache(const model::APlayerCameraManager& pcm) {
		if (!pcm.is_valid())
			return;
		auto view_opt = pcm.TryGetCameraView();
		if (!view_opt)
			return;
		const auto& v = *view_opt;
		const bool ok = !(v.Location.X == 0.0 && v.Location.Y == 0.0 && v.Location.Z == 0.0)
			&& v.FOV > 0.0f && v.FOV <= 180.0f;
		if (!ok)
			return;
		std::lock_guard<std::mutex> lock(m_cameraCacheMutex);
		m_cameraCache.LastValidLocation = v.Location;
		m_cameraCache.LastValidRotation = v.Rotation;
		m_cameraCache.LastValidFOV = v.FOV;
		m_cameraCache.HasValidCache = true;
		m_cameraCache.timestamp = std::chrono::steady_clock::now();
	}

	model::FVector2D CoreEngine::ProjectWorldToScreen(model::FVector WorldLocation) {
		model::FVector2D Screenlocation{};


        model::FMinimalViewInfo s_cam{};
        {
            std::lock_guard<std::mutex> lock(m_cameraCacheMutex);
            if (!m_cameraCache.HasValidCache)
                return {};
            const auto age = std::chrono::steady_clock::now() - m_cameraCache.timestamp;
            if (age > std::chrono::milliseconds(700))
                return {};
			s_cam.Location = m_cameraCache.LastValidLocation;
			s_cam.Rotation = m_cameraCache.LastValidRotation;
			s_cam.FOV = m_cameraCache.LastValidFOV;
		}

		auto CameraLocation = s_cam.Location;
		auto CameraRotation = s_cam.Rotation;
		auto CameraFOV = s_cam.FOV;

        auto matrix = model::Math::toMatrix(CameraRotation, {});

        model::FVector vAxisX = model::FVector(matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]);
        model::FVector vAxisY = model::FVector(matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]);
        model::FVector vAxisZ = model::FVector(matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]);

        model::FVector vDelta = WorldLocation - CameraLocation;
        model::FVector vTransformed = model::FVector(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

        if (vTransformed.Z < 1.0)
            vTransformed.Z = 1.0;

        if (CameraFOV <= 0.0 || CameraFOV > 180.0) {
            return {};
        }




        const float aspect = (m_screen_height > 0.f) ? (m_screen_width / m_screen_height) : 1.f;
        const float fov_x = CameraFOV * (float)model::Constants::HALF_DEG_TO_RAD;
        const float fov_y = atanf(tanf(fov_x) / aspect);

        Screenlocation.X = (double)(m_screen_width / 2.f) + vTransformed.X * ((double)(m_screen_width / 2.f) / (double)tanf(fov_x)) / vTransformed.Z;
        Screenlocation.Y = (double)(m_screen_height / 2.f) - vTransformed.Y * ((double)(m_screen_height / 2.f) / (double)tanf(fov_y)) / vTransformed.Z;
        if(Screenlocation.X < 0 || Screenlocation.X > m_screen_width ||
            Screenlocation.Y < 0 || Screenlocation.Y > m_screen_height) {
            return {};
		}

        return Screenlocation;
    }

    std::string CoreEngine::GetAgentName(std::string name) const {
        if (name.empty())
            return xorstr_("");

        static std::unordered_map<std::string, std::string> patterns = {
        { xorstr_("Sequoia_PC_C"),                              xorstr_("Iso") },
        { xorstr_("Pawn_TrainingBot_DanceHall_Easy_Attacker"),  xorstr_("Bot Training") },
        { xorstr_("TrainingBot_PC_C"),                          xorstr_("Bot") },
        { xorstr_("Rift_PC_C"),                                 xorstr_("Astra") },
        { xorstr_("Rift_TargetingForm_PC_C"),                   xorstr_("Astra") },
        { xorstr_("Breach_PC_C"),                               xorstr_("Breach") },
        { xorstr_("Sarge_PC_C"),                                xorstr_("Brimstone") },
        { xorstr_("Deadeye_PC_C"),                              xorstr_("Chamber") },
        { xorstr_("Gumshoe_PC_C"),                              xorstr_("Cypher") },
        { xorstr_("Wushu_PC_C"),                                xorstr_("Jett") },
        { xorstr_("Grenadier_PC_C"),                            xorstr_("Kay/O") },
        { xorstr_("Killjoy_PC_C"),                              xorstr_("Killjoy") },
        { xorstr_("Sprinter_PC_C"),                             xorstr_("Neon") },
        { xorstr_("Wraith_PC_C"),                               xorstr_("Omen") },
        { xorstr_("Phoenix_PC_C"),                              xorstr_("Phoenix") },
        { xorstr_("Clay_PC_C"),                                 xorstr_("Raze") },
        { xorstr_("Vampire_PC_C"),                              xorstr_("Reyna") },
        { xorstr_("Thorne_PC_C"),                               xorstr_("Sage") },
        { xorstr_("Guide_PC_C"),                                xorstr_("Skye") },
        { xorstr_("BountyHunter_PC_C"),                         xorstr_("Fade") },
        { xorstr_("Hunter_PC_C"),                               xorstr_("Sova") },
        { xorstr_("Pandemic_PC_C"),                             xorstr_("Viper") },
        { xorstr_("Stealth_PC_C"),                              xorstr_("Yoru") },
        { xorstr_("Mage_PC_C"),                                 xorstr_("Harbor") },
        { xorstr_("AggroBot_PC_C"),                             xorstr_("Gekko") },
        { xorstr_("Cable_PC_C"),                                xorstr_("Deadlock") },
        { xorstr_("Smonk_PC_C"),                                xorstr_("Clove") },
        { xorstr_("Nox_PC_C"),                                  xorstr_("Vyse") },
        { xorstr_("Cashew_PC_C"),                               xorstr_("Tejo") },
        { xorstr_("Terra_PC_C"),                                xorstr_("Waylay") }
        };


        auto itp = patterns.find(name);
        if (itp != patterns.end()) return itp->second;
        static const std::unordered_map<std::string, std::string> kPlain = {
            {"iso","Iso"},{"astra","Astra"},{"breach","Breach"},{"brimstone","Brimstone"},
            {"chamber","Chamber"},{"cypher","Cypher"},{"jett","Jett"},{"kayo","Kay/O"},
            {"kay/o","Kay/O"},{"killjoy","Killjoy"},{"neon","Neon"},{"omen","Omen"},
            {"phoenix","Phoenix"},{"raze","Raze"},{"reyna","Reyna"},{"sage","Sage"},
            {"skye","Skye"},{"fade","Fade"},{"sova","Sova"},{"viper","Viper"},
            {"yoru","Yoru"},{"harbor","Harbor"},{"gekko","Gekko"},{"deadlock","Deadlock"},
            {"clove","Clove"},{"vyse","Vyse"},{"tejo","Tejo"},{"waylay","Waylay"},
            {"bot","Bot"}
        };
        std::string low;
        low.reserve(name.size());
        for (unsigned char c : name) low.push_back((char)std::tolower(c));
        if (low.size() > 2 && low.compare(low.size() - 2, 2, "_c") == 0)
            low.erase(low.size() - 2);
        auto itp2 = kPlain.find(low);
        if (itp2 != kPlain.end()) return itp2->second;
        return xorstr_("");
    }


    std::string CoreEngine::MapWeaponName(const std::string& raw) {
        if (raw.empty()) return {};

        static const std::vector<std::pair<std::string, std::string>> kWeapons = {
            { "basepistol",          "Classic" },
            { "sawedoffshotgun",     "Shorty" },
            { "automaticpistol",     "Frenzy" },
            { "lugerpistol",         "Ghost" },
            { "revolverpistol",      "Sheriff" },
            { "vector",              "Stinger" },
            { "submachinegun_mp5",   "Spectre" },
            { "pumpshotgun",         "Bucky" },
            { "automaticshotgun",    "Judge" },
            { "assaultrifle_burst",  "Bulldog" },
            { "dmr",                 "Guardian" },
            { "assaultrifle_acr",    "Phantom" },
            { "assaultrifle_ak",     "Vandal" },
            { "leversniperrifle",    "Marshal" },
            { "boltsniper",          "Operator" },
            { "lightmachinegun",     "Ares" },
            { "heavymachinegun",     "Odin" },
            { "ability_melee_base",  "Knife" },
            { "gun_sprinter_x_heavylightninggun_production", "Neon X" },
            { "ability_wushu_x_dagger_production",           "Jett Dagger" }
        };

        std::string lower = raw;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        for (const auto& [prefix, display] : kWeapons) {
            if (lower.find(prefix) == 0)
                return display;
        }

        std::string out = raw;
        auto cut = out.find_last_of('_');
        if (cut != std::string::npos && cut > 0) {
            const std::string suffix = out.substr(cut);
            if (suffix.size() <= 3 && (suffix == "_C" || suffix == "_0" || suffix == "_1"))
                out = out.substr(0, cut);
        }
        return out;
    }

std::string CoreEngine::GetSkillName(std::string name) const {
        if (name.empty())
            return xorstr_("");

        static std::unordered_map<std::string, std::string> skill_patterns = {

        { xorstr_("Patch_Phoenix_MolotovFire_C"),                       xorstr_("Hot Hands") },
        { xorstr_("Projectile_Phoenix_E_FlareCurve_Synced_C"),          xorstr_("Curve Ball") },
        { xorstr_("GameObject_Phoenix_X_ResTarget_Production_C"),       xorstr_("Phoenix Ult") },


        { xorstr_("GameObject_Rift_X_Markers_C"),                       xorstr_("Astra Star") },
        { xorstr_("GameObject_Rift_4_BlackHole_C"),                     xorstr_("Gravity Well") },
        { xorstr_("Ability_Rift_Q_FlashBurst_C"),                       xorstr_("Nova Pulse") },
        { xorstr_("Ability_Rift_E_TransformRift_Smoke_C"),              xorstr_("Nebula") },


        { xorstr_("Projectile_Breach_Q_ThroughWalls_Flash_C"),         xorstr_("Flashpoint") },
        { xorstr_("Ability_Breach_4_FusionBlast_C"),                   xorstr_("Aftershock") },
        { xorstr_("GameObject_Breach_E_SweetSpotFissure_C"),           xorstr_("Fault Line") },
        { xorstr_("GameObject_Breach_X_Shockwave_C"),                  xorstr_("Rolling Thunder") },


        { xorstr_("GameObject_Sarge_E_SpeedStim_C"),                   xorstr_("Stim Beacon") },
        { xorstr_("Ability_Sarge_Q_Molotov_Production_C"),             xorstr_("Incendiary") },
        { xorstr_("Ability_Sarge_4_MapTargetSmoke_Production_C"),      xorstr_("Sky Smoke") },
        { xorstr_("GameObject_Sarge_X_OrbitalStrike_Production_C"),    xorstr_("Orbital Strike") },


        { xorstr_("GameObect_Deadeye_E_Trap_C"),                         xorstr_("Trademark") },
        { xorstr_("GameObject_Deadeye_E_Teleporter_Tether_C"),         xorstr_("Rendezvous") },


        { xorstr_("Projectile_Smonk_DecayNade_C"),                     xorstr_("Meddle Nade") },
        { xorstr_("GameObject_Smonk_NewSmoke_C"),                      xorstr_("Ruse Smoke") },


        { xorstr_("GameObject_Gumshoe_E_TripWire_C"),                  xorstr_("Tripwire 1") },
        { xorstr_("GameObject_Gumshoe_E_TripWire_SecondWire_C"),       xorstr_("Tripwire 2") },
        { xorstr_("Projectile_Gumshoe_4_CageTrap_C"),                  xorstr_("Cyber Cage") },
        { xorstr_("Ability_Gumshoe_Q_Camera_C"),                       xorstr_("Spycam") },


        { xorstr_("GameObject_StealthingTrap_SoundSensor_C"),          xorstr_("Sonic Sensor") },
        { xorstr_("Projectile_NetToss_C"),                             xorstr_("Gravnet") },


        { xorstr_("Projectile_E_BountyHunter_Divebomb_C"),             xorstr_("Haunt Projectile") },
        { xorstr_("GameObject_BountyHunter_E_LoSReveal_Source_Reactivate_C"), xorstr_("Haunt") },
        { xorstr_("Projectile_Q_BountyHunter_TetherGrenade_SphereExpansion_C"), xorstr_("Seize Projectile") },


        { xorstr_("Projectile_E_Aggrobot_DiscTurret_PowerWave_C"),     xorstr_("Dizzy") },
        { xorstr_("Projectile_Aggrobot_C_ExplodeyPatch_C"),            xorstr_("Mosh Pit") },


        { xorstr_("Zone_Mage_Q_SphereShield_C"),                       xorstr_("Cove") },


        { xorstr_("Projectile_Wushu_4_Smoke_C"),                       xorstr_("Cloudburst Projectile") },
        { xorstr_("GameObject_Wushu_4_SmokeZone_C"),                   xorstr_("Cloudburst") },


        { xorstr_("Projectile_Grenadier_E_SuppressionBlade_C"),        xorstr_("Zero Point Blade") },
        { xorstr_("Ability_E_Grenadier_EMPKnife_C"),                   xorstr_("Zero Point Blade") },
        { xorstr_("Ability_Grenadier_C_Flash_C"),                      xorstr_("Flash Drive") },
        { xorstr_("Ability_Grenadier_Q_BasicSemtex_C"),                xorstr_("Fragment") },


        { xorstr_("Projectile_Killjoy_4_RemoteBees_MultiDetonate_C"),  xorstr_("Nanoswarm") },
        { xorstr_("Ability_Killjoy_Q_Alarmbot_C"),                     xorstr_("Alarmbot") },
        { xorstr_("Pawn_Killjoy_E_Turret_C"),                          xorstr_("Turret") },


        { xorstr_("Zone_Wraith_4_Smoke_C"),                            xorstr_("Dark Cover") },
        { xorstr_("Projectile_Wraith_Q_NearsightMissile_C"),           xorstr_("Paranoia") },


        { xorstr_("Projectile_Vampire_4_NearsightAoE_C"),              xorstr_("Leer") },


        { xorstr_("FXC_Hunter_E_DronePossessed_C"),                        xorstr_("Owl Drone") },
        { xorstr_("Projectile_Hunter_4_ExplosiveBolt_PrototypeBalance_C"), xorstr_("Shock Bolt") },
        { xorstr_("Projectile_Hunter_Q_RevealBolt_C"),                 xorstr_("Recon Bolt") },


        { xorstr_("Projectile_Cashew_Q_ShellShockGrenade_C"),          xorstr_("Special Delivery") },


        { xorstr_("GameObject_Nox_BarbedWire_C"),                      xorstr_("Barbwire") },
        { xorstr_("GameObject_Nox_WallTrap_C"),                        xorstr_("Wall Trap") },
        { xorstr_("GameObject_Nox_StealthingTrap_Flash_2_C"),          xorstr_("Arc Rose") }
        };

        return skill_patterns[name];
    }

}
