#include <include/common.hpp>
#include <include/core/iapplication.hpp>
#include <include/utils/logger.hpp>
#include <include/utils/config.hpp>
#include <include/game/engine/core_engine.hpp>
#include <include/game/combat.hpp>
#include <include/game/trigger_control.hpp>
#include <include/game/navigation.hpp>
#include <include/driver/idriver.hpp>
#include <include/render/render.hpp>
#include <include/core/actors_data_provider.hpp>
#include <include/discord_hook/injector.hpp>
#include <include/discord_hook/hook_shared.h>
#include <include/discord_overlay/overlay_server.hpp>
#include <cstring>

#pragma comment(lib, "winmm.lib")

namespace maxi::core {
    using namespace maxi::utils;
	using namespace maxi::game::engine;

    namespace {
        float to_float(double value) {
            return static_cast<float>(value);
        }

        std::string narrow_for_log(const std::wstring& value) {
            std::string result;
            result.reserve(value.size());
            for (wchar_t ch : value) {
                result.push_back(static_cast<char>(ch));
            }
            return result;
        }
    }

    


    class Application : public IApplication {
    public:
        Application() : m_running(false), m_paused(false) {}

        ~Application() override {
            shutdown();
        }

        bool initialize() override {
            try {
                LOG_INFO("STEP 1: initialize_configuration...");
                if (!initialize_configuration()) {
                    LOG_ERROR("Failed to initialize configuration");
                    return false;
                }
                LOG_INFO("STEP 1: OK");

                bool gui_only = (wcsstr(GetCommandLineW(), L"--gui-only") != nullptr);

                if (!gui_only) {
                    LOG_INFO("STEP 2: create_driver...");
                    maxi::driver::g_driver = maxi::driver::create_driver();
                    if (!maxi::driver::g_driver) {
                        LOG_ERROR("Failed to create driver instance");
                        return false;
                    }
                    LOG_INFO("STEP 2: OK");

                    LOG_INFO("STEP 3: driver->setup() (auto-loads driver)...");
                    if (!maxi::driver::g_driver->setup()) {
                        LOG_WARNING("Failed to setup driver");
                        return false;
                    }
                    LOG_INFO("STEP 3: OK - driver running in kernel");
                } else {
                    LOG_INFO("Running in GUI-only / Standalone mode");
                }

                if (!gui_only) {
                    LOG_INFO("STEP 4: CoreEngine initialize (Attaching to target process)...");
                    if (!m_core_engine->initialize()) {
                        LOG_ERROR("Failed to initialize CoreEngine");
                        return false;
                    }
                    LOG_INFO("STEP 4: OK");

                    LOG_INFO("STEP 5: Lineups init...");
                    if (!initialize_navigation()) {
                        LOG_WARNING("Failed to initialize navigation system");
                    }
                    LOG_INFO("STEP 5: OK");
                }




                maxi::discord_overlay::set_config_callback(&Application::overlay_set_config);
                maxi::discord_overlay::install_discord_overlay_html();
                maxi::discord_overlay::start_overlay_server(47777);

                LOG_INFO("STEP 6: initialize_graphics (D3D11 + ImGui + Overlay Hijacking)...");
                if (!initialize_graphics()) {
                    LOG_WARNING("Failed to initialize graphics (D3D + ImGui)");
                    return false;
                }
                LOG_INFO("STEP 6: OK");

                LOG_INFO("ALL STEPS DONE - initialization complete");
                return true;
            }
            catch (const std::exception& e) {
                LOG_ERROR("Exception during initialization: " + std::string(e.what()));
                return false;
            }
        }

        void run() override {
            bool gui_only = (wcsstr(GetCommandLineW(), L"--gui-only") != nullptr);
            m_running = true;

            if (!gui_only) {
                if (!m_core_engine) {
                    LOG_ERROR("CoreEngine not available");
                    return;
                }

                m_core_engine->start();



                LOG_INFO("[READ-ONLY] Combat thread skipping - ESP read-only mod");
            }

            this->ui_main_loop();
        }



        void full_unload() {
            if (m_unload_pending) return;
            LOG_WARNING("[UNLOAD] Full unload requested - will close at end of frame...");
            m_unload_pending = true;
            m_running = false;
        }


        void teardown_for_exit() {
            LOG_WARNING("[UNLOAD] Full unload started...");
            m_running = false;
            stop();


            if (m_core_engine) {
                m_core_engine->stop();
            }
            maxi::game::combat::m_combat->stop_thread();


            if (m_core_engine) {
                m_core_engine->shutdown();
            }


            DWORD discord_pid = 0;
            {
                HWND h = FindWindowW(L"DiscordOverlayInputTrap", nullptr);
                if (!h) h = FindWindowW(L"ExternalDiscordHWND", nullptr);
                if (h) {
                    DWORD pid_tmp = 0;
                    GetWindowThreadProcessId(h, &pid_tmp);
                    discord_pid = pid_tmp;
                }
            }
            if (discord_pid) {
                LOG_INFO(std::format("[UNLOAD] Discord PID {} -> ejecting DLL", discord_pid));
                maxi::discord_hook::Injector::EjectDiscordHook(discord_pid);
            }


            cleanup_graphics();

            if (maxi::driver::g_driver && maxi::driver::g_driver->is_valid()) {
                LOG_WARNING("[UNLOAD] Calling driver unload...");
                maxi::driver::g_driver->unload();
                LOG_WARNING("[UNLOAD] Driver unload completed");
            }
            maxi::driver::g_driver.reset();

            LOG_WARNING("[UNLOAD] All resources released. You can exit the process.");
        }

        void shutdown() override {
            stop();

            maxi::game::combat::m_combat->stop_thread();

            if (m_core_engine) {
                m_core_engine->shutdown();
                m_core_engine.reset();
            }

            cleanup_graphics();

            m_running = false;
        }

        bool is_running() const override {
            return m_running;
        }

        void set_target_process(const std::wstring& process_name) override {
            if (!m_core_engine) {
                LOG_ERROR("CoreEngine not available");
                return;
            }
            if (!m_core_engine->set_target_process(process_name)) {
                LOG_ERROR("Failed to attach to process: " + narrow_for_log(process_name));
            } else {
                LOG_INFO("Successfully attached to process: " + narrow_for_log(process_name));
            }
        }

        void set_configuration(const std::string& config_name, const std::string& value) override {
            if (maxi::utils::g_config) {
                maxi::utils::g_config->set_value(config_name, maxi::utils::ConfigValue(value));
            }
        }

        void pause() override {
            m_paused = true;
            if (m_core_engine) {
                m_core_engine->pause();
            }
        }

        void resume() override {
            m_paused = false;
            if (m_core_engine) {
                m_core_engine->resume();
            }
        }

        void stop() override {
            m_running = false;
            if (m_core_engine) {
                m_core_engine->stop();
            }
        }

    private:
        maxi::game::engine::ActorsData                   m_actors_data;


        bool m_running;
        bool m_paused;
        bool m_unload_pending = false;

        bool initialize_configuration() {
            try {

                maxi::utils::g_config->set_value(xorstr_("driver.service_name"), maxi::utils::ConfigValue(xorstr_("gdrv_svc")));
                maxi::utils::g_config->set_value(xorstr_("driver.device_path_template"), maxi::utils::ConfigValue(xorstr_("\\\\.\\{computer_name}")));
                maxi::utils::g_config->set_value(xorstr_("game.target_process"), maxi::utils::ConfigValue(maxi::driver::target_process_name()));
                maxi::utils::g_config->set_value(xorstr_("logging.level"), maxi::utils::ConfigValue(xorstr_("INFO")));
                var->config.combat.enabled = false;
                var->config.trigger_control.enabled = false;
                LOG_INFO("[READ-ONLY] ESP read-only mode: Aimbot/Triggerbot forcibly disabled");
                return true;
            }
            catch (const std::exception& e) {
                LOG_ERROR("Failed to initialize configuration: " + std::string(e.what()));
                return false;
            }
        }

        bool initialize_navigation() {
            try {

                if (!maxi::game::navigation::g_navigation->load_navigation()) {
                    LOG_ERROR("Failed to load navigation");
                    return false;
                }

                return true;
            }
            catch (const std::exception& e) {
                LOG_ERROR("Exception initializing navigation: " + std::string(e.what()));
                return false;
            }
        }


        bool initialize_graphics() {
            return maxi::render::m_render->init();
        }

        void cleanup_graphics() {
            if (maxi::render::m_render) {
                maxi::render::m_render->shutdown();
                maxi::render::m_render.reset();
            }
        }

        void draw_radar() {
            if (!var->config.visuals.radar) return;
            auto actors_data = m_core_engine->get_actors_data();
            if (actors_data.players_list.empty()) return;
            auto camera_view = m_core_engine->get_cached_camera_view();
            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            const float size = var->config.visuals.radar_size;
            const float range = var->config.visuals.radar_range;
            if (size < 80.f || range < 5.f) return;
            const ImVec2 org(20.0f, 60.0f);
            const ImVec2 ctr(org.x + size * 0.5f, org.y + size * 0.5f);
            const float R = size * 0.5f;
            draw->AddRectFilled(org, ImVec2(org.x + size, org.y + size), IM_COL32(0, 0, 0, 140));
            draw->AddRect(org, ImVec2(org.x + size, org.y + size), IM_COL32(255, 255, 255, 120));
            const double yaw = (double)camera_view.Rotation.Yaw * 3.141592653589793 / 180.0;
            const double cy = std::cos(yaw), sy = std::sin(yaw);
            const auto& cam = camera_view.Location;
            draw->AddTriangleFilled(ImVec2(ctr.x, ctr.y - 6.f), ImVec2(ctr.x - 5.f, ctr.y + 5.f),
                ImVec2(ctr.x + 5.f, ctr.y + 5.f), IM_COL32(255, 255, 255, 255));
            const auto& vc = var->config.visuals.visible_color;
            const auto& nc = var->config.visuals.not_visible_color;
            const ImU32 col_vis = IM_COL32(vc[0] * 255.f, vc[1] * 255.f, vc[2] * 255.f, 255);
            const ImU32 col_hid = IM_COL32(nc[0] * 255.f, nc[1] * 255.f, nc[2] * 255.f, 255);
            const ImU32 col_team = IM_COL32(0, 255, 255, 255);
            for (const auto& actor : actors_data.players_list) {
                if (!actor.aux_resolved) continue;
                if (actor.hidden) continue;
                if (var->config.visuals.ignore_dormant && actor.is_dormant) continue;
                if (actor.health <= 0.f) continue;
                if (actor.is_teammate && !var->config.visuals.team_check) continue;
                const auto& p = actor.world_bones[0];
                if (p.X == 0.0 && p.Y == 0.0 && p.Z == 0.0) continue;
                const double dx = (p.X - cam.X) / 100.0;
                const double dy = (p.Y - cam.Y) / 100.0;
                if (dx * dx + dy * dy > (double)range * (double)range) continue;
                const double rx = dx * cy + dy * sy;
                const double ry = -dx * sy + dy * cy;
                const float px = (float)(ctr.x + rx / range * R);
                const float py = (float)(ctr.y - ry / range * R);
                const ImU32 c = actor.is_teammate ? col_team : (actor.visible ? col_vis : col_hid);
                draw->AddCircleFilled(ImVec2(px, py), 3.5f, c, 12);
            }
        }

        void draw_fov_circle() {
            if (!var->config.visuals.show_fov) return;
            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            const float cx = (float)(maxi::render::m_render->Width / 2.0f);
            const float cy = (float)(maxi::render::m_render->Height / 2.0f);
            const auto& c = var->config.visuals.fov_color;
            draw->AddCircle(ImVec2(cx, cy), var->config.visuals.fov_radius,
                IM_COL32(c[0] * 255.f, c[1] * 255.f, c[2] * 255.f, c[3] * 255.f), 64, 1.5f);
        }

        void draw_players() {
            static thread_local uint64_t s_copyfail = 0;
            auto world_data = m_core_engine->get_world_data();
            auto actors_data = m_core_engine->get_actors_data();

            if (maxi::render::m_render) {
                const float screen_w = static_cast<float>(maxi::render::m_render->Width);
                const float screen_h = static_cast<float>(maxi::render::m_render->Height);


                if (m_core_engine) {
                    m_core_engine->set_screen_dimensions(screen_w, screen_h);
                }
            }


            static thread_local decltype(actors_data) s_last_good;
            bool used_stale = false;
            if (actors_data.players_list.empty()) {
                if (!s_last_good.players_list.empty()) { actors_data = s_last_good; used_stale = true; ++s_copyfail; }
                else return;
            } else {
                s_last_good = actors_data;
            }

			auto camera_view = m_core_engine->get_cached_camera_view();
            static bool s_bone_diag_logged = false;
            const uint64_t frame_now = GetTickCount64();
            static uint64_t s_diag_n = 0;
            const bool diag_on = (++s_diag_n % 400) == 1;
            for (const auto& actor : actors_data.players_list) {
                auto base = actor.actor.get_base_address();

                std::array<maxi::game::model::FVector2D, 17> bones{};
                if (actor.world_bones[0].Z == 0.0f && actor.world_bones[0].X == 0.0f && actor.world_bones[0].Y == 0.0f) {
                    if (diag_on) { const auto& p = actor.world_bones[0]; LOG_WARNING(std::format("[DRAWDIAG] {} SKIP nobone dist={:.0f}m hp={:.0f}", actor.name.empty() ? "??" : actor.name, (camera_view.Location.Distance(actor.world_bones[0])) / 100.0, actor.health)); }
                    continue;
                }

                if (!actor.aux_resolved) {
                    if (diag_on) LOG_WARNING(std::format("[DRAWDIAG] {} SKIP noaux dist={:.0f}m", actor.name.empty() ? "??" : actor.name, (camera_view.Location.Distance(actor.world_bones[0])) / 100.0));
                    continue;
                }
                if (actor.hidden)
                    continue;
                if (var->config.visuals.ignore_dormant && actor.is_dormant)
                    continue;


                {
                    const uint64_t bage = (actor.have_vel && frame_now >= actor.bone_ms)
                        ? (frame_now - actor.bone_ms) : (uint64_t)-1;
                    const bool fresh_hp = actor.health_ms != 0 && frame_now >= actor.health_ms
                        && (frame_now - actor.health_ms) <= 3000;
                    const bool fresh_bones = (bage <= 3000ULL);
                    const bool usable_bones = (bage <= 30000ULL);
                    if (!fresh_bones && !(fresh_hp && usable_bones)) continue;
                    if (actor.bones_synth && bage > 2000ULL) continue;
                }
                if (actor.health <= 0.f) {
                    if (diag_on) LOG_WARNING(std::format("[DRAWDIAG] {} SKIP dead dist={:.0f}m", actor.name.empty() ? "??" : actor.name, (camera_view.Location.Distance(actor.world_bones[0])) / 100.0));
                    continue;
                }
                if (diag_on) LOG_WARNING(std::format("[DRAWDIAG] {} DRAW dist={:.0f}m vis={} hp={:.0f} synth={}",
                    actor.name.empty() ? "??" : actor.name, (camera_view.Location.Distance(actor.world_bones[0])) / 100.0,
                    actor.visible ? 1 : 0, actor.health, actor.bones_synth ? 1 : 0));
                std::array<maxi::game::model::FVector, 17> wb = actor.world_bones;
                if (actor.have_vel) {
                    const double dt = (double)(GetTickCount64() - actor.bone_ms) / 1000.0;

                    if (dt > 0.0 && dt < 1.0) {
                        const double hz0 = (actor.bone_interval > 5.f && actor.bone_interval < 500.f)
                            ? (double)actor.bone_interval / 1000.0 : 0.05;


                        const double hz = hz0 * 0.7;
                        const double h = dt < hz ? dt : hz;
                        constexpr double CAP = 250.0;
                        auto off = actor.bone_vel * h;
                        const double m = std::sqrt(off.X * off.X + off.Y * off.Y + off.Z * off.Z);
                        if (m > CAP) off = off * (CAP / m);
                        for (auto& b : wb) b += off;
                    }
                }
                {
                    static thread_local uint64_t s_age_n = 0;
                    static thread_local uint64_t s_novel = 0;
                    static thread_local uint64_t s_over100 = 0;
                    static thread_local uint64_t s_synthn = 0;
                    static thread_local uint64_t s_synth_old = 0, s_real_old = 0;
                    static thread_local double s_age_sum = 0.0, s_age_max = 0.0;
                    if (actor.have_vel) {
                        const double age = (double)(frame_now >= actor.bone_ms ? frame_now - actor.bone_ms : 0);
                        s_age_sum += age; if (age > s_age_max) s_age_max = age;
                        if (age > 100.0) ++s_over100;
                        if (actor.bones_synth) {
                            ++s_synthn;
                            if (age > 1000.0) {
                                ++s_synth_old;

                                static std::unordered_set<uintptr_t> s_so;
                                if (s_so.insert(base).second) {
                                    LOG_WARNING(std::format("[SYNTHOLD] name={} bage={:.0f}ms hp={:.0f} dorm={} vis={} ally={}",
                                        actor.name.empty() ? "??" : actor.name, age, actor.health,
                                        actor.is_dormant ? 1 : 0, actor.visible ? 1 : 0, actor.is_teammate ? 1 : 0));
                                }
                            }
                        }
                        else if (age > 120.0) {
                            ++s_real_old;

                            static std::unordered_set<uintptr_t> s_ro;
                            if (s_ro.insert(base).second) {
                                LOG_WARNING(std::format("[REALOLD] name={} bage={:.0f}ms hp={:.0f} vis={} dorm={} same={}",
                                    actor.name.empty() ? "??" : actor.name, age, actor.health,
                                    actor.visible ? 1 : 0, actor.is_dormant ? 1 : 0, actor.bones_synth ? 1 : 0));
                            }
                        }
                        ++s_age_n;
                    } else { ++s_novel; }
                    if (s_age_n + s_novel >= 600) {
                        LOG_INFO(std::format("[AGE] n={} novel={} over100={} copyfail={} synth={} synth_old={} real_old={} cam={}ms bone_avg={:.0f}ms bone_max={:.0f}ms (600-draw avg)",
                            s_age_n, s_novel, s_over100, s_copyfail, s_synthn, s_synth_old, s_real_old,
                            m_core_engine->camera_age_ms(),
                            s_age_n ? s_age_sum / (double)s_age_n : 0.0, s_age_max));
                        for (const auto& aa : actors_data.players_list) {
                            const uint64_t ab = (aa.have_vel && frame_now >= aa.bone_ms)
                                ? (frame_now - aa.bone_ms) : 0;
                            LOG_WARNING(std::format("[ACT] {} age={}ms hp={:.0f} vis={} dorm={} synth={} ally={} hid={} aux={} mesh=0x{:X} ba=0x{:X}",
                                aa.name.empty() ? "??" : aa.name, ab, aa.health,
                                aa.visible ? 1 : 0, aa.is_dormant ? 1 : 0, aa.bones_synth ? 1 : 0,
                                aa.is_teammate ? 1 : 0, aa.hidden ? 1 : 0, aa.aux_resolved ? 1 : 0,
                                (unsigned long long)aa.mesh, (unsigned long long)aa.bone_array));
                        }
                        s_age_n = 0; s_novel = 0; s_over100 = 0; s_copyfail = 0; s_synthn = 0;
                        s_synth_old = 0; s_real_old = 0; s_age_sum = 0.0; s_age_max = 0.0;
                    }
                }
                for (size_t bi = 0; bi < bones.size(); ++bi)
                    bones[bi] = m_core_engine->ProjectWorldToScreen(wb[bi]);

                if (!s_bone_diag_logged && !actor.name.empty() && actor.world_bones[0].Z != 0.0) {
                    s_bone_diag_logged = true;
                    const auto& w = actor.world_bones;
                    double dist = camera_view.Location.Distance(w[0]) / 100.0;
                    double h2n = w[1].Z - w[0].Z;
                    double h2f = w[0].Z - w[13].Z;
                    LOG_INFO(std::format("[BONE DIAG] name={} cam_z={:.0f} | head_world=({:.0f},{:.0f},{:.0f}) neck=({:.0f},{:.0f},{:.0f}) foot=({:.0f},{:.0f},{:.0f}) | dist={:.1f}m headNeckH={:.0f} headFootH={:.0f} | screen_headY={:.0f} screen_footY={:.0f}",
                        actor.name, camera_view.Location.Z,
                        w[0].X, w[0].Y, w[0].Z, w[1].X, w[1].Y, w[1].Z, w[13].X, w[13].Y, w[13].Z,
                        dist, h2n, h2f, bones[0].Y, bones[13].Y));
                    std::string zbuf;
                    for (size_t zi = 0; zi < w.size(); ++zi)
                        zbuf += std::format(" {}:{:.0f}", zi, w[zi].Z);
                    LOG_INFO(std::format("[BONE Z] all17={}", zbuf));
                }

                auto health = actor.health;
                if (health <= 0.f)
                    continue;




                float HeadToNeck = to_float(bones[1].Y - bones[0].Y);


                float pTop = to_float(bones[0].Y) - (HeadToNeck * 0.35f);


                float pBottom = to_float(bones[13].Y) + (HeadToNeck * 0.05f);


                float pHeight = pBottom - pTop;


                float pWidth = pHeight * 0.32f;


                float centerX = to_float((bones[3].X + bones[6].X) / 2.0);

                if (!var->config.visuals.enabled)
                    continue;



                const bool is_teammate = actor.is_teammate;

                if (is_teammate && !var->config.visuals.team_check)
                    continue;

                auto color = var->config.visuals.visible_check ?
                    (actor.visible ? var->config.visuals.visible_color : var->config.visuals.not_visible_color) :
                    var->config.visuals.visible_color;

                if (is_teammate) {
                    color[0] = 0.0f; color[1] = 1.0f; color[2] = 1.0f; color[3] = 1.0f;
                }


                if (var->config.visuals.rainbow) {
                    float hue = fmodf((float)GetTickCount64() * 0.0005f + (float)base * 0.0000001f, 1.0f);
                    float r = 0, g = 0, b = 0;
                    float h6 = hue * 6.0f;
                    float x = 1.0f - std::abs(fmodf(h6, 2.0f) - 1.0f);
                    if (h6 < 1.0f)      { r = 1.0f; g = x; b = 0.0f; }
                    else if (h6 < 2.0f) { r = x; g = 1.0f; b = 0.0f; }
                    else if (h6 < 3.0f) { r = 0.0f; g = 1.0f; b = x; }
                    else if (h6 < 4.0f) { r = 0.0f; g = x; b = 1.0f; }
                    else if (h6 < 5.0f) { r = x; g = 0.0f; b = 1.0f; }
                    else                { r = 1.0f; g = 0.0f; b = x; }
                    color[0] = r; color[1] = g; color[2] = b; color[3] = 1.0f;
                }


                bool hasHead = m_core_engine->inScreen(bones[0]);
                bool hasNeck = m_core_engine->inScreen(bones[1]);
                bool hasShoulders = m_core_engine->inScreen(bones[3]) && m_core_engine->inScreen(bones[6]);
                bool hasFeet = m_core_engine->inScreen(bones[13]);
                bool hasChest = m_core_engine->inScreen(bones[2]);


                if (var->config.visuals.health && hasFeet && hasHead && hasNeck && hasShoulders) {


                    const int tpos_h = var->config.visuals.text_position;
                    float healthBarX = centerX - pWidth / 2.0f;
                    float healthBarY = pBottom + 3.0f;
                    float healthBarWidth = pWidth;
                    float healthBarHeight = 4.0f;

                    if (tpos_h == 1) {
                        healthBarY = pTop - 3.0f - 4.0f;
                    }
                    else if (tpos_h == 2) {
                        healthBarX = centerX - pWidth / 2.0f - 5.0f;
                        healthBarY = pTop;
                        healthBarWidth = 4.0f;
                        healthBarHeight = pHeight;
                    }
                    else if (tpos_h == 3) {
                        healthBarX = centerX + pWidth / 2.0f + 1.0f;
                        healthBarY = pTop;
                        healthBarWidth = 4.0f;
                        healthBarHeight = pHeight;
                    }

                    float shield = 0.0f;
                    float shieldMax = 50.0f;
                    if (actor.damage_handler.is_valid()) {
                        shield = actor.damage_handler.GetShield();
                        const float ms = actor.damage_handler.GetMaxShield();
                        if (ms > 0.0f) shieldMax = ms;
                    }
                    if (shield > shieldMax) shieldMax = shield;

                    if (shield > 0.0f) {
                        float shieldBarY = healthBarY + healthBarHeight + 2.0f;
                        float shW = (shield / shieldMax) * healthBarWidth;
                        shW = std::clamp(shW, 0.0f, healthBarWidth);
                        ImGui::GetBackgroundDrawList()->AddRectFilled(
                            ImVec2(healthBarX, shieldBarY), ImVec2(healthBarX + healthBarWidth, shieldBarY + 3.0f),
                            IM_COL32(20, 20, 20, 200));
                        ImGui::GetBackgroundDrawList()->AddRectFilled(
                            ImVec2(healthBarX, shieldBarY), ImVec2(healthBarX + shW, shieldBarY + 3.0f),
                            IM_COL32(90, 120, 255, 255));
                    }

                    ImGuiID health_id = ImHashData(&base, sizeof(base)) + 1;

                    maxi::render::m_render->HorizontalHealthBar(
                        healthBarX, healthBarY, healthBarWidth, healthBarHeight,
                        static_cast<int>(health), 100, health_id
                    );
                }


                if (var->config.visuals.box && (hasHead || hasFeet)) {
                    ImGuiID box_id = ImHashData(&base, sizeof(base)) + 2;
                    maxi::render::m_render->CorneredBox(
                        ImVec2(centerX, pBottom), pWidth, pTop, pBottom,
                        IM_COL32(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f),
                        var->config.visuals.box_thickness, true, health, box_id, 3.0f
                    );
                }


                if (var->config.visuals.box3d && hasHead && hasFeet) {
                    const auto& hw = actor.world_bones[0];
                    const auto& fw = actor.world_bones[13];
                    const double hw_half = pWidth;
                    const double hw_d = hw_half / 100.0;
                    const double fw_d = hw_d;
                    const double zTop = hw.Z + 20.0;
                    const double zBot = fw.Z;
                    maxi::game::model::FVector corners[8] = {                        { fw.X + fw_d, fw.Y - fw_d, zBot }, { fw.X - fw_d, fw.Y - fw_d, zBot },
                        { fw.X - fw_d, fw.Y + fw_d, zBot }, { fw.X + fw_d, fw.Y + fw_d, zBot },
                        { hw.X + hw_d, hw.Y - hw_d, zTop }, { hw.X - hw_d, hw.Y - hw_d, zTop },
                        { hw.X - hw_d, hw.Y + hw_d, zTop }, { hw.X + hw_d, hw.Y + hw_d, zTop },
                    };
                    ImVec2 sc[8];
                    bool ok3d = true;
                    for (int i = 0; i < 8; ++i) {
                        auto p = m_core_engine->ProjectWorldToScreen(corners[i]);
                        sc[i] = ImVec2(to_float(p.X), to_float(p.Y));
                        if (!m_core_engine->inScreen(p)) { ok3d = false; break; }
                    }
                    if (ok3d) {
                        const int edge[12][2] = {
                            {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4},
                            {0,4},{1,5},{2,6},{3,7}
                        };
                        auto* drawlist = ImGui::GetBackgroundDrawList();
                        ImU32 col = IM_COL32(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f);
                        for (int e = 0; e < 12; ++e)
                            drawlist->AddLine(sc[edge[e][0]], sc[edge[e][1]], col, var->config.visuals.box_thickness);
                    }
                }



                if (var->config.visuals.skeleton && !actor.bones_synth) {





                    static const std::array<std::pair<int, int>, 15> skel = { {
                        {10,9}, {9,2}, {2,1}, {1,0},
                        {3,6},
                        {3,4},  {4,5},
                        {6,7},  {7,8},
                        {11,10},{12,11},{13,12},
                        {14,10},{15,14},{16,15}
                    } };
                    const ImU32 skel_col = IM_COL32(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f);
                    for (const auto& link : skel) {
                        const auto& p0 = bones[link.first];
                        const auto& p1 = bones[link.second];
                        if (m_core_engine->inScreen(p0) && m_core_engine->inScreen(p1)) {


                            ImVec2 a0(to_float(p0.X), to_float(p0.Y));
                            ImVec2 a1(to_float(p1.X), to_float(p1.Y));
                            const bool is_foot_link =
                                (link.first == 13 || link.first == 16 || link.second == 13 || link.second == 16);
                            if (is_foot_link) {
                                a1 = ImVec2(a0.x + (a1.x - a0.x) * 0.82f, a0.y + (a1.y - a0.y) * 0.82f);
                            }
                            maxi::render::m_render->DrawSkeletonLine(
                                a0, a1,
                                skel_col,
                                var->config.visuals.skeleton_thickness,
                                0.0f
                            );
                        }
                    }
                }

                if (var->config.visuals.show_all_bones) {
                    auto* bonedraw = ImGui::GetBackgroundDrawList();
                    const ImU32 bcol = IM_COL32(255, 255, 0, 230);
                    for (size_t bi = 0; bi < bones.size(); ++bi) {
                        if (!bones[bi].isValid() || !m_core_engine->inScreen(bones[bi])) continue;
                        bonedraw->AddCircleFilled(
                            ImVec2(to_float(bones[bi].X), to_float(bones[bi].Y)),
                            2.5f, bcol, 12);
                        bonedraw->AddText(
                            ImVec2(to_float(bones[bi].X) + 4.f, to_float(bones[bi].Y) - 6.f),
                            bcol, std::to_string(bi).c_str());
                    }
                }


                if (var->config.visuals.head) {
                    if (hasHead) {
                        maxi::render::m_render->DrawCircleFilled(
                            ImVec2(to_float(bones[0].X), to_float(bones[0].Y)),
                            3.5f,
                            IM_COL32(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f),
                            30
                        );
                    }
				}


                if (var->config.visuals.view_angle && actor.has_facing && hasHead) {
                    const double yr = (double)actor.facing_yaw * 3.141592653589793 / 180.0;
                    const maxi::game::model::FVector endw{
                        wb[0].X + std::cos(yr) * 260.0,
                        wb[0].Y + std::sin(yr) * 260.0,
                        wb[0].Z };
                    const auto end2 = m_core_engine->ProjectWorldToScreen(endw);
                    maxi::render::m_render->DrawLine(
                        ImVec2(to_float(bones[0].X), to_float(bones[0].Y)),
                        ImVec2(to_float(end2.X), to_float(end2.Y)),
                        IM_COL32(0, 255, 0, 255), 1.5f);
                }


                if (var->config.visuals.china_hat && hasHead) {
                    const auto& hw = actor.world_bones[0];
                    float hr = 1.f, hg = 1.f, hb = 1.f;
                    if (var->config.visuals.rainbow) {
                        const float hue = fmodf((float)GetTickCount64() * 0.00025f, 1.0f);
                        const float h6 = hue * 6.0f;
                        const float x = 1.0f - std::abs(fmodf(h6, 2.0f) - 1.0f);
                        if (h6 < 1.0f)      { hr = 1.0f; hg = x; hb = 0.0f; }
                        else if (h6 < 2.0f) { hr = x; hg = 1.0f; hb = 0.0f; }
                        else if (h6 < 3.0f) { hr = 0.0f; hg = 1.0f; hb = x; }
                        else if (h6 < 4.0f) { hr = 0.0f; hg = x; hb = 1.0f; }
                        else if (h6 < 5.0f) { hr = x; hg = 0.0f; hb = 1.0f; }
                        else                { hr = 1.0f; hg = 0.0f; hb = x; }
                    } else {
                        hr = var->config.visuals.china_hat_color[0];
                        hg = var->config.visuals.china_hat_color[1];
                        hb = var->config.visuals.china_hat_color[2];
                    }
                    const ImU32 hat_col = IM_COL32(hr * 255.f, hg * 255.f, hb * 255.f, 255);
                    const maxi::game::model::FVector apex_w{ hw.X, hw.Y, hw.Z + 13.0 };
                    const auto apex = m_core_engine->ProjectWorldToScreen(apex_w);
                    constexpr int HAT_SEG = 12;
                    maxi::game::model::FVector2D prev{};
                    bool have_prev = false;
                    for (int hi = 0; hi <= HAT_SEG; ++hi) {
                        const double ang = (double)hi / (double)HAT_SEG * 6.283185307179586;
                        const maxi::game::model::FVector bw{
                            hw.X + cos(ang) * 25.0, hw.Y + sin(ang) * 25.0, hw.Z };
                        const auto b2 = m_core_engine->ProjectWorldToScreen(bw);
                        maxi::render::m_render->DrawLine(
                            ImVec2(to_float(apex.X), to_float(apex.Y)),
                            ImVec2(to_float(b2.X), to_float(b2.Y)), hat_col, 1.5f);
                        if (have_prev)
                            maxi::render::m_render->DrawLine(
                                ImVec2(to_float(prev.X), to_float(prev.Y)),
                                ImVec2(to_float(b2.X), to_float(b2.Y)), hat_col, 1.0f);
                        prev = b2;
                        have_prev = true;
                    }
                }


                if (var->config.visuals.lines && hasFeet && centerX > -50.f && centerX < 99999.f) {
                    ImVec2 feet_center = ImVec2(centerX, pBottom);
                    maxi::render::m_render->DrawTracerLine(
                        feet_center,
                        IM_COL32(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f),
                        var->config.visuals.lines_thickness
                    );
                }


                if (var->config.visuals.agent_name && !actor.name.empty() && hasFeet && hasShoulders) {


                    const int tpos = var->config.visuals.text_position;
                    const bool txt_bottom = (tpos == 0);
                    const bool txt_top = (tpos == 1);
                    const bool txt_left = (tpos == 2);
                    const bool txt_right = (tpos == 3);


                    ImVec2 text_anchor(centerX, 0.f);
                    if (txt_bottom) {
                        text_anchor.y = var->config.visuals.health ? (pBottom + 3.0f + 4.0f + 5.0f) : (pBottom + 3.0f);
                    }
                    else if (txt_top) {
                        text_anchor.y = pTop - 5.0f;
                    }
                    else if (txt_left) {
                        text_anchor.y = (pTop + pBottom) * 0.5f;
                        text_anchor.x = centerX - pWidth / 2.0f - 6.0f;
                    }
                    else {
                        text_anchor.y = (pTop + pBottom) * 0.5f;
                        text_anchor.x = centerX + pWidth / 2.0f + 6.0f;
                    }

                    std::string display_name = actor.name;

                    if (var->config.visuals.distance && hasChest) {
                        maxi::game::model::FVector pos = actor.world_bones[2];
                        auto distance = camera_view.Location.Distance(pos) / 100.0;
                        int rounded_distance = (int)std::round(distance);
                        display_name = std::format("[{}m] {}", rounded_distance, actor.name);
                    }

                    maxi::render::m_render->DrawPlayerName(
                        text_anchor, display_name,
                        IM_COL32(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f)
                    );

                    if (var->config.visuals.weapon && !actor.weapon_name.empty()) {
                        ImVec2 w_anchor = text_anchor;
                        if (txt_bottom)      w_anchor.y += 14.0f;
                        else if (txt_top)    w_anchor.y -= 14.0f;
                        else if (txt_left)   w_anchor.x -= 40.0f;
                        else                 w_anchor.x += 40.0f;
                        maxi::render::m_render->DrawPlayerName(
                            w_anchor, actor.weapon_name,
                            IM_COL32(255, 220, 120, 255)
                        );
                    }
                }
            }
        }




        static void overlay_set_config(const std::string& key, bool v) {
            if (!var) return;
            auto& vis = var->config.visuals;
            if (key == "box") vis.box = v;
            else if (key == "skeleton") vis.skeleton = v;
            else if (key == "agent_name") vis.agent_name = v;
            else if (key == "health") vis.health = v;
            else if (key == "radar") vis.radar = v;
            else if (key == "team_check") vis.team_check = v;
            else if (key == "visible_check") vis.visible_check = v;
            else if (key == "ignore_dormant") vis.ignore_dormant = v;
            else if (key == "show_bots") vis.show_bots = v;
            else if (key == "view_angle") vis.view_angle = v;
            else if (key == "skills") vis.skills = v;
            LOG_INFO(std::format("[OVERLAY MENU] {} = {}", key, v ? 1 : 0));
        }

        void publish_discord_frame() {

            if (!m_core_engine || !maxi::render::m_render)
                return;


            m_core_engine->set_screen_dimensions(
                static_cast<float>(maxi::render::m_render->Width),
                static_cast<float>(maxi::render::m_render->Height));

            static uint64_t s_frame_id = 0;
            ::discord_hook::SharedFrameData frame{};
            frame.frame_id = ++s_frame_id;
            frame.screen_width = static_cast<uint32_t>(maxi::render::m_render->Width);
            frame.screen_height = static_cast<uint32_t>(maxi::render::m_render->Height);
            frame.show_menu = var->gui.enabled;
            frame.show_boxes = var->config.visuals.box;
            frame.show_skeleton = var->config.visuals.skeleton;
            frame.show_names = var->config.visuals.agent_name;
            frame.show_health = var->config.visuals.health;
            frame.menu_scale = 1.0f;
            frame.actor_count = 0;

            auto actors_data = m_core_engine->get_actors_data();
            for (const auto& actor : actors_data.players_list) {
                if (frame.actor_count >= 64)
                    break;
                if (actor.world_bones[0].X == 0.0 && actor.world_bones[0].Y == 0.0 && actor.world_bones[0].Z == 0.0)
                    continue;
                std::array<maxi::game::model::FVector2D, 17> b{};
                for (size_t bi = 0; bi < b.size(); ++bi)
                    b[bi] = m_core_engine->ProjectWorldToScreen(actor.world_bones[bi]);

                auto& out = frame.actors[frame.actor_count];
                out.head_x = to_float(b[0].X);   out.head_y = to_float(b[0].Y);
                out.neck_x = to_float(b[1].X);   out.neck_y = to_float(b[1].Y);
                out.chest_x = to_float(b[2].X);  out.chest_y = to_float(b[2].Y);
                out.pelvis_x = to_float(b[10].X); out.pelvis_y = to_float(b[10].Y);
                out.l_shoulder_x = to_float(b[3].X);  out.l_shoulder_y = to_float(b[3].Y);
                out.l_elbow_x = to_float(b[4].X);     out.l_elbow_y = to_float(b[4].Y);
                out.l_wrist_x = to_float(b[5].X);     out.l_wrist_y = to_float(b[5].Y);
                out.r_shoulder_x = to_float(b[6].X);  out.r_shoulder_y = to_float(b[6].Y);
                out.r_elbow_x = to_float(b[7].X);     out.r_elbow_y = to_float(b[7].Y);
                out.r_wrist_x = to_float(b[8].X);     out.r_wrist_y = to_float(b[8].Y);
                out.l_hip_x = to_float(b[11].X);   out.l_hip_y = to_float(b[11].Y);
                out.l_knee_x = to_float(b[12].X);  out.l_knee_y = to_float(b[12].Y);
                out.l_ankle_x = to_float(b[13].X); out.l_ankle_y = to_float(b[13].Y);
                out.r_hip_x = to_float(b[14].X);   out.r_hip_y = to_float(b[14].Y);
                out.r_knee_x = to_float(b[15].X);  out.r_knee_y = to_float(b[15].Y);
                out.r_ankle_x = to_float(b[16].X); out.r_ankle_y = to_float(b[16].Y);
                out.screen_w = static_cast<float>(maxi::render::m_render->Width);
                out.screen_h = static_cast<float>(maxi::render::m_render->Height);
                out.team_id = actor.is_teammate ? 1u : 2u;
                out.is_visible = actor.visible;
                out.is_valid = !actor.name.empty() && actor.health > 0.f;
                std::strncpy(out.agent_name, actor.name.c_str(), sizeof(out.agent_name) - 1);
                out.agent_name[sizeof(out.agent_name) - 1] = '\0';
                frame.actor_count++;
            }

            if (maxi::render::m_render->is_hook_active())
                maxi::discord_hook::Injector::WriteSharedFrameData(&frame, sizeof(frame));



            {
                std::string js = std::format("{{\"w\":{},\"h\":{},\"actors\":[",
                    frame.screen_width, frame.screen_height);
                for (uint32_t i = 0; i < frame.actor_count; ++i) {
                    const auto& a = frame.actors[i];
                    if (i) js += ',';
                    js += std::format("{{\"name\":\"{}\",\"team\":{},\"vis\":{},\"bones\":[",
                        a.agent_name, a.team_id, a.is_visible ? "true" : "false");
                    const float* p = &a.head_x;


                    for (int bi = 0; bi < 16; ++bi) {
                        if (bi) js += ',';
                        js += std::format("[{:.1f},{:.1f}]", p[bi * 2], p[bi * 2 + 1]);
                    }
                    js += "]}";
                }
                js += "],\"menu\":{\"show\":";
                js += var->gui.enabled ? "true" : "false";
                js += ",\"items\":[";
                {
                    auto& v2 = var->config.visuals;
                    const std::pair<const char*, bool> items[] = {
                        {"box", v2.box}, {"skeleton", v2.skeleton}, {"agent_name", v2.agent_name},
                        {"health", v2.health}, {"radar", v2.radar}, {"team_check", v2.team_check},
                        {"visible_check", v2.visible_check}, {"ignore_dormant", v2.ignore_dormant},
                        {"show_bots", v2.show_bots}, {"view_angle", v2.view_angle}, {"skills", v2.skills},
                    };
                    bool first = true;
                    for (auto& it : items) {
                        if (!first) js += ',';
                        first = false;
                        js += std::format("{{\"k\":\"{}\",\"v\":{}}}", it.first, it.second ? "true" : "false");
                    }
                }
                js += "]}}";
                maxi::discord_overlay::set_frame_json(std::move(js));
            }

            static uint64_t s_pub_n = 0;
            if ((++s_pub_n % 300) == 1)
                LOG_INFO(std::format("[PUBLISH] frame={} actors={} screen={}x{}",
                    s_frame_id, frame.actor_count, frame.screen_width, frame.screen_height));
        }

        ImU32 draw_timer_bar(double time_remaining, double defuse_progress, int32_t defuse_section) {
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* draw = ImGui::GetBackgroundDrawList();


            const float circle_radius = 40.0f;
            const float circle_thickness = 6.0f;
            const float max_time = 45.0f;


            ImVec2 circle_center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.25f);


            float time_percent = std::max(0.0f, std::min(1.0f, (float)time_remaining / max_time));


            ImU32 progress_color = IM_COL32(
                (int)(255 * (1.0f - time_percent)),
                (int)(255 * time_percent),
                0,
                255
            );


            draw->AddCircle(circle_center, circle_radius, IM_COL32(60, 60, 60, 200), 64, circle_thickness);


            if (time_percent > 0.0f) {
                const float PI = 3.14159265359f;
                float start_angle = -PI * 0.5f;
                float end_angle = start_angle + (2.0f * PI * time_percent);


                draw->PathClear();
                draw->PathArcTo(circle_center, circle_radius, start_angle, end_angle, 64);
                draw->PathStroke(progress_color, false, circle_thickness);
            }


            const float defuse_radius = circle_radius + 15.0f;
            const float defuse_thickness = 4.0f;
            const float PI = 3.14159265359f;


            ImU32 defuse_color = IM_COL32(100, 150, 255, 255);
            ImU32 defuse_bg_color = IM_COL32(120, 120, 120, 180);


            float gap_angle = 0.15f;
            float section_angle = PI - gap_angle;


            float section1_start = -PI * 0.5f + gap_angle * 0.5f;
            float section1_end = section1_start + section_angle;
            draw->PathClear();
            draw->PathArcTo(circle_center, defuse_radius, section1_start, section1_end, 32);
            draw->PathStroke(defuse_bg_color, false, defuse_thickness);


            float section2_start = PI * 0.5f + gap_angle * 0.5f;
            float section2_end = section2_start + section_angle;
            draw->PathClear();
            draw->PathArcTo(circle_center, defuse_radius, section2_start, section2_end, 32);
            draw->PathStroke(defuse_bg_color, false, defuse_thickness);


            if (defuse_section >= 0 && defuse_progress > 0.0f) {

                float normalized_progress = std::max(0.0f, std::min(100.0f, (float)defuse_progress)) / 100.0f;

                if (defuse_section == 0) {


                    float section_progress = std::min(1.0f, normalized_progress * 2.0f);
                    float progress_end = section1_start + section_angle * section_progress;
                    draw->PathClear();
                    draw->PathArcTo(circle_center, defuse_radius, section1_start, progress_end, 32);
                    draw->PathStroke(defuse_color, false, defuse_thickness);
                } else if (defuse_section == 1) {

                    draw->PathClear();
                    draw->PathArcTo(circle_center, defuse_radius, section1_start, section1_end, 32);
                    draw->PathStroke(defuse_color, false, defuse_thickness);



                    float section_progress = std::max(0.0f, (normalized_progress - 0.5f) * 2.0f);
                    float progress_end = section2_start + section_angle * section_progress;
                    draw->PathClear();
                    draw->PathArcTo(circle_center, defuse_radius, section2_start, progress_end, 32);
                    draw->PathStroke(defuse_color, false, defuse_thickness);
                } else if (defuse_section >= 2) {

                    draw->PathClear();
                    draw->PathArcTo(circle_center, defuse_radius, section1_start, section1_end, 32);
                    draw->PathStroke(defuse_color, false, defuse_thickness);

                    draw->PathClear();
                    draw->PathArcTo(circle_center, defuse_radius, section2_start, section2_end, 32);
                    draw->PathStroke(defuse_color, false, defuse_thickness);
                }
            }


            std::string timer_text = std::format("{}", (int)time_remaining);
            ImVec2 text_size = ImGui::CalcTextSize(timer_text.c_str());
            ImVec2 text_pos = ImVec2(
                circle_center.x - text_size.x * 0.5f,
                circle_center.y - text_size.y * 0.5f
            );


            draw->AddText(
                ImVec2(text_pos.x + 1, text_pos.y + 1),
                IM_COL32(0, 0, 0, 255),
                timer_text.c_str()
            );


            draw->AddText(
                text_pos,
                IM_COL32(255, 255, 255, 255),
                timer_text.c_str()
            );

            return progress_color;
        }

        void draw_navigation() {


            return;
            if (!var->config.visuals.nav_helper ||
                !maxi::game::navigation::g_navigation ||
                !maxi::game::navigation::g_navigation->has_navigation()) {
                return;
            }


            {
                static ULONGLONG s_nav_gate_t = 0;
                const ULONGLONG nav_now = GetTickCount64();
                if (nav_now - s_nav_gate_t < 200) return;
                s_nav_gate_t = nav_now;
            }

            const double max_display_distance_meters = std::clamp(
                static_cast<double>(var->config.visuals.nav_display_distance), 1.0, 30.0);
            const double aim_guide_distance_meters = std::clamp(
                static_cast<double>(var->config.visuals.nav_aim_guide_distance),
                0.25,
                max_display_distance_meters);
            const double standing_position_tolerance_meters = std::clamp(
                static_cast<double>(var->config.visuals.nav_position_tolerance),
                0.1,
                aim_guide_distance_meters);
            const float crosshair_snap_distance_pixels = std::clamp(
                var->config.visuals.nav_crosshair_tolerance, 0.5f, 20.0f);

            auto world_data = m_core_engine->get_world_data();


            auto camera_view = m_core_engine->get_cached_camera_view();
            maxi::game::model::FVector player_position = camera_view.Location;


            auto navigation = maxi::game::navigation::g_navigation->get_navigation(world_data.local_player_name, world_data.world.get_name_string(), world_data.acknowledged_pawn.CurrentEquippable().get_name_string());


            std::map<std::pair<int, int>, int> position_map;


            struct OccupiedTooltip {
                ImVec2 position;
                ImVec2 size;
                ImVec2 min_bounds() const { return position; }
                ImVec2 max_bounds() const { return ImVec2(position.x + size.x, position.y + size.y); }
            };
            std::vector<OccupiedTooltip> occupied_tooltips;



            bool any_crosshair_near_aimpoint = false;
            float crossX = (float)(maxi::render::m_render->Width / 2.0f);
            float crossY = (float)(maxi::render::m_render->Height / 2.0f);

            for (const auto& nav_check : navigation) {
                maxi::game::model::FVector nav_pos_check{
                    static_cast<float>(nav_check.position_x),
                    static_cast<float>(nav_check.position_y),
                    static_cast<float>(nav_check.position_z)
                };

                double distance_units_check = player_position.Distance(nav_pos_check);
                double distance_meters_check = distance_units_check / 100.0;
                if (distance_meters_check > aim_guide_distance_meters) continue;



                maxi::game::model::FVector aim_point_check{
                    static_cast<float>(nav_check.rotation_x),
                    static_cast<float>(nav_check.rotation_y),
                    static_cast<float>(nav_check.rotation_z)
                };
                auto aim_screen_pos_check = m_core_engine->ProjectWorldToScreen(aim_point_check);
                if (!m_core_engine->inScreen(aim_screen_pos_check)) continue;


                float xdist = to_float(aim_screen_pos_check.X) - crossX;
                float ydist = to_float(aim_screen_pos_check.Y) - crossY;
                float crosshair_distance_check = sqrtf((xdist * xdist) + (ydist * ydist));

                if (crosshair_distance_check <= crosshair_snap_distance_pixels) {
                    any_crosshair_near_aimpoint = true;
                    break;
                }
            }


            for (const auto& navpoint : navigation) {

                maxi::game::model::FVector nav_pos{
                    static_cast<float>(navpoint.position_x),
                    static_cast<float>(navpoint.position_y),
                    static_cast<float>(navpoint.position_z)
                };

                double distance_units = player_position.Distance(nav_pos);
                double distance_meters = distance_units / 100.0;


                if (distance_meters > max_display_distance_meters) continue;


                maxi::game::model::FVector nav_position{
                    static_cast<float>(navpoint.position_x),
                    static_cast<float>(navpoint.position_y),
                    static_cast<float>(navpoint.position_z)
                };


                maxi::game::model::FVector aim_point{
                    static_cast<float>(navpoint.rotation_x),
                    static_cast<float>(navpoint.rotation_y),
                    static_cast<float>(navpoint.rotation_z)
                };

                auto nav_screen_pos = m_core_engine->ProjectWorldToScreen(nav_position);
                auto aim_screen_pos = m_core_engine->ProjectWorldToScreen(aim_point);


                {

                    {
                        ImDrawList* draw = ImGui::GetBackgroundDrawList();


                        const int num_segments = 32;
                        const float radius_world = 20.0f;
                        std::vector<ImVec2> circle_points;
                        bool all_points_visible = true;

                        for (int i = 0; i < num_segments; i++) {
                            float angle = (float)i / (float)num_segments * 2.0f * 3.14159265359f;


                            maxi::game::model::FVector circle_point{
                                static_cast<float>(navpoint.position_x + cos(angle) * radius_world),
                                static_cast<float>(navpoint.position_y + sin(angle) * radius_world),
                                static_cast<float>(navpoint.position_z - 100.0)
                            };


                            auto screen_point = m_core_engine->ProjectWorldToScreen(circle_point);

                            if (m_core_engine->inScreen(screen_point)) {
                                circle_points.push_back(ImVec2(to_float(screen_point.X), to_float(screen_point.Y)));
                            }
                            else {
                                all_points_visible = false;
                                break;
                            }
                        }


                        if (all_points_visible && circle_points.size() == num_segments) {

                            ImU32 border_color = (distance_meters <= standing_position_tolerance_meters) ?
                                IM_COL32(0, 255, 0, 220) :
                                IM_COL32(255, 255, 0, 220);


                            for (int i = 0; i < num_segments; i++) {
                                int next_i = (i + 1) % num_segments;
                                draw->AddLine(
                                    circle_points[i],
                                    circle_points[next_i],
                                    border_color,
                                    2.5f
                                );
                            }
                        }
                    }


                    if (m_core_engine->inScreen(nav_screen_pos)) {

                        int screen_x = static_cast<int>(nav_screen_pos.X / 50) * 50;
                        int screen_y = static_cast<int>(nav_screen_pos.Y / 50) * 50;
                        std::pair<int, int> position_key = { screen_x, screen_y };


                        int vertical_step = position_map[position_key] * 60;
                        position_map[position_key]++;


                        std::string nav_info = std::format(
                            "{}\n{}m",
                            navpoint.tips.empty() ? (world_data.local_player_name + " NavPoint") : navpoint.tips,
                            (int)std::round(distance_meters)
                        );

                        maxi::render::m_render->DrawPlayerName(
                            ImVec2(to_float(nav_screen_pos.X), to_float(nav_screen_pos.Y + 10 + vertical_step)),
                            nav_info,
                            IM_COL32(200, 255, 200, 255)
                        );
                    }

                    if (distance_meters <= aim_guide_distance_meters) {
                        ImDrawList* draw = ImGui::GetBackgroundDrawList();

                        ImVec2 screen_center = ImVec2(crossX, crossY);


                        if (m_core_engine->inScreen(aim_screen_pos)) {
                            ImVec2 aim_screen_pt = ImVec2(to_float(aim_screen_pos.X), to_float(aim_screen_pos.Y));


                            float xdist = to_float(aim_screen_pos.X) - crossX;
                            float ydist = to_float(aim_screen_pos.Y) - crossY;
                            float crosshair_distance = sqrtf((xdist * xdist) + (ydist * ydist));
                            bool crosshair_near_this_aimpoint = crosshair_distance <= crosshair_snap_distance_pixels;

                            if (any_crosshair_near_aimpoint) {


                                if (crosshair_near_this_aimpoint) {
                                    draw->AddCircleFilled(
                                        aim_screen_pt,
                                        1.8f,
                                        IM_COL32(0, 255, 0, 255)
                                    );
                                }
                            }
                            else {

                                draw->AddLine(
                                    screen_center,
                                    aim_screen_pt,
                                    IM_COL32(255, 255, 255, 200),
                                    2.f
                                );


                                draw->AddCircleFilled(
                                    aim_screen_pt,
                                    1.8f,
                                    IM_COL32(255, 0, 0, 255)
                                );


                                if (!navpoint.tips.empty()) {

                                    ImVec2 text_size = ImGui::CalcTextSize(navpoint.tips.c_str());
                                    float padding = 8.0f;
                                    float tooltip_width = text_size.x + (padding * 2);
                                    float tooltip_height = text_size.y + (padding * 2);


                                    auto check_collision = [&](ImVec2 pos, float width, float height) -> bool {
                                        const float margin_x = 15.0f;
                                        const float margin_y = 10.0f;

                                        for (const auto& occupied : occupied_tooltips) {

                                            if (pos.x < occupied.max_bounds().x + margin_x &&
                                                pos.x + width + margin_x > occupied.min_bounds().x &&
                                                pos.y < occupied.max_bounds().y + margin_y &&
                                                pos.y + height + margin_y > occupied.min_bounds().y) {
                                                return true;
                                            }
                                        }
                                        return false;
                                        };


                                    std::vector<ImVec2> candidate_positions;


                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X + 35), to_float(aim_screen_pos.Y - 10)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X - tooltip_width - 35), to_float(aim_screen_pos.Y - 10)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X + 15), to_float(aim_screen_pos.Y - tooltip_height - 25)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X + 15), to_float(aim_screen_pos.Y + 25)));


                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X + 30), to_float(aim_screen_pos.Y - tooltip_height - 20)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X + 30), to_float(aim_screen_pos.Y + 20)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X - tooltip_width - 30), to_float(aim_screen_pos.Y - tooltip_height - 20)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X - tooltip_width - 30), to_float(aim_screen_pos.Y + 20)));


                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X + 60), to_float(aim_screen_pos.Y - 5)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X - tooltip_width - 60), to_float(aim_screen_pos.Y - 5)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X), to_float(aim_screen_pos.Y - tooltip_height - 40)));
                                    candidate_positions.push_back(ImVec2(to_float(aim_screen_pos.X), to_float(aim_screen_pos.Y + 40)));


                                    ImVec2 tooltip_pos = candidate_positions[0];
                                    bool found_position = false;

                                    for (const auto& candidate : candidate_positions) {
                                        if (!check_collision(candidate, tooltip_width, tooltip_height)) {
                                            tooltip_pos = candidate;
                                            found_position = true;
                                            break;
                                        }
                                    }


                                    if (!found_position) {
                                        for (int radius = 80; radius <= 200; radius += 20) {
                                            for (int angle = 0; angle < 360; angle += 45) {
                                                float rad = angle * 3.14159f / 180.0f;
                                                ImVec2 spiral_pos = ImVec2(
                                                    to_float(aim_screen_pos.X + cos(rad) * radius - tooltip_width * 0.5f),
                                                    to_float(aim_screen_pos.Y + sin(rad) * radius - tooltip_height * 0.5f)
                                                );

                                                if (!check_collision(spiral_pos, tooltip_width, tooltip_height)) {
                                                    tooltip_pos = spiral_pos;
                                                    found_position = true;
                                                    break;
                                                }
                                            }
                                            if (found_position) break;
                                        }
                                    }


                                    occupied_tooltips.push_back({
                                        tooltip_pos,
                                        ImVec2(tooltip_width, tooltip_height)
                                        });


                                    ImVec2 balloon_min = ImVec2(tooltip_pos.x - padding, tooltip_pos.y - padding);
                                    ImVec2 balloon_max = ImVec2(tooltip_pos.x + text_size.x + padding, tooltip_pos.y + text_size.y + padding);


                                    draw->AddRectFilled(
                                        balloon_min, balloon_max,
                                        IM_COL32(0, 0, 0, 200),
                                        4.0f
                                    );


                                    draw->AddRect(
                                        balloon_min, balloon_max,
                                        IM_COL32(255, 255, 255, 150),
                                        4.0f, 0, 1.0f
                                    );



                                    ImVec2 balloon_center = ImVec2(
                                        tooltip_pos.x + text_size.x * 0.5f,
                                        tooltip_pos.y + text_size.y * 0.5f
                                    );


                                    ImVec2 tail_start;
                                    if (aim_screen_pos.X < balloon_center.x) {

                                        tail_start = ImVec2(balloon_min.x, balloon_center.y);
                                    }
                                    else {

                                        tail_start = ImVec2(balloon_max.x, balloon_center.y);
                                    }

                                    ImVec2 tail_end = ImVec2(to_float(aim_screen_pos.X), to_float(aim_screen_pos.Y));


                                    float tail_distance = sqrtf(powf(tail_end.x - tail_start.x, 2.0f) + powf(tail_end.y - tail_start.y, 2.0f));
                                    if (tail_distance < 150.0f) {

                                        ImVec2 tail_top = ImVec2(tail_start.x, tail_start.y - 3.0f);
                                        ImVec2 tail_bottom = ImVec2(tail_start.x, tail_start.y + 3.0f);

                                        draw->AddTriangleFilled(tail_top, tail_bottom, tail_end, IM_COL32(0, 0, 0, 200));
                                        draw->AddTriangle(tail_top, tail_bottom, tail_end, IM_COL32(255, 255, 255, 150), 1.0f);
                                    }


                                    draw->AddText(
                                        tooltip_pos,
                                        IM_COL32(255, 255, 255, 255),
                                        navpoint.tips.c_str()
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }

        void draw_timing_bar() {

            return;
            if (!var->config.visuals.timing_bar) return;

            static bool s_last_key = false;
            static float s_elapsed = 0.0f;
            static float s_bar_duration = 1.5f;
            static bool s_active = false;
            static ULONGLONG s_last_tick = 0;

            const ULONGLONG now_ms = GetTickCount64();
            const float dt = s_last_tick ? (float)(now_ms - s_last_tick) / 1000.0f : 0.0f;
            s_last_tick = now_ms;

            bool any_key = ((GetAsyncKeyState('Q') & 0x8000) != 0) ||
                           ((GetAsyncKeyState('E') & 0x8000) != 0) ||
                           ((GetAsyncKeyState('C') & 0x8000) != 0) ||
                           ((GetAsyncKeyState('X') & 0x8000) != 0);
            bool key_down_edge = any_key && !s_last_key;
            s_last_key = any_key;

            if (key_down_edge) {
                float found = 0.0f;
                {
                    auto wd = m_core_engine->get_world_data();
                    const auto& ln = wd.local_player_name;

                    if (ln == "Sova")       found = 1.2f;
                    else if (ln == "Breach") found = 1.8f;
                    else if (ln == "Brimstone") found = 2.0f;
                    else if (ln == "Raze")  found = 1.4f;
                    else if (ln == "Kay/O") found = 1.6f;
                    if (found <= 0.0f) found = 1.5f;

                    if (!ln.empty() && maxi::game::navigation::g_navigation &&
                        maxi::game::navigation::g_navigation->has_navigation()) {
                        auto navs = maxi::game::navigation::g_navigation->get_navigation(
                            ln, wd.world.get_name_string(),
                            wd.acknowledged_pawn.CurrentEquippable().get_name_string());
                        maxi::game::model::FVector cam = m_core_engine->get_cached_camera_view().Location;
                        for (const auto& np : navs) {
                            maxi::game::model::FVector p{ (float)np.position_x, (float)np.position_y, (float)np.position_z };
                            double d = cam.Distance(p) / 100.0;
                            if (d < 8.0 && np.timing > 0.0) { found = (float)np.timing; break; }
                        }
                    }
                }
                s_bar_duration = std::max(0.3f, found);
                s_elapsed = 0.0f;
                s_active = true;
            }

            if (!s_active) return;


            s_elapsed += dt;
            const float progress = std::clamp(s_elapsed / s_bar_duration, 0.0f, 1.0f);
            if (s_elapsed >= s_bar_duration) {
                s_active = false;
                return;
            }

            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            const float w = var->config.visuals.timing_bar_width;
            const float h = var->config.visuals.timing_bar_height;
            const float cx = (float)(maxi::render::m_render->Width / 2.0f);
            const float cy = (float)(maxi::render::m_render->Height / 2.0f);
            const ImVec2 bar_min(cx - w * 0.5f, cy + 40.0f);
            const ImVec2 bar_max(cx + w * 0.5f, cy + 40.0f + h);


            draw->AddRectFilled(bar_min, bar_max, IM_COL32(0, 0, 0, 160), 4.0f);
            draw->AddRect(bar_min, bar_max, IM_COL32(255, 255, 255, 200), 4.0f, 0, 1.0f);


            ImU32 fill_col = IM_COL32(70, 140, 255, 255);
            if (progress > 0.7f) fill_col = IM_COL32(0, 255, 120, 255);
            if (progress > 0.0f) {
                ImVec2 fill_max(bar_min.x + (bar_max.x - bar_min.x) * progress, bar_max.y);
                draw->AddRectFilled(bar_min, fill_max, fill_col, 4.0f);
            }

            char buf[64];
            float remaining = s_bar_duration - s_elapsed;
            if (remaining < 0.0f) remaining = 0.0f;
            snprintf(buf, sizeof(buf), "%.1fs", remaining);
            draw->AddText(ImVec2(cx - ImGui::CalcTextSize(buf).x * 0.5f, bar_min.y - 24.0f),
                IM_COL32(255, 255, 255, 220), buf);
        }

        void draw_skills() {

            {
                static ULONGLONG s_skills_gate_t = 0;
                const ULONGLONG now = GetTickCount64();
                if (now - s_skills_gate_t < 200) return;
                s_skills_gate_t = now;
            }

            auto world_data = m_core_engine->get_world_data();
            auto actors_data = m_core_engine->get_skills_data();
            if (var->config.visuals.objective && actors_data.timed_bomb.is_valid()) {
                auto relative_location = actors_data.timed_bomb.RelativeLocation();
                auto position = m_core_engine->ProjectWorldToScreen(relative_location);
                auto time_to_explode_opt = maxi::driver::g_driver->try_read<double>(actors_data.timed_bomb.get_base_address() + memory_map::BombTimer);
				auto defuse_time_opt = maxi::driver::g_driver->try_read<double>(actors_data.timed_bomb.get_base_address() + memory_map::BombDefuseProgress);
				auto defuse_section_opt = maxi::driver::g_driver->try_read<int32_t>(actors_data.timed_bomb.get_base_address() + memory_map::DefuseStage);
                
                if (!time_to_explode_opt || !defuse_time_opt || !defuse_section_opt) {

                } else {
                    double time_to_explode = *time_to_explode_opt;
					double defuse_time = *defuse_time_opt * 100 / 6.984602;
					int32_t defuse_section = *defuse_section_opt;

					ImU32 timer_color = draw_timer_bar(time_to_explode, defuse_time, defuse_section);

					if (m_core_engine->inScreen(position)) {
						maxi::render::m_render->DrawPlayerName(ImVec2(to_float(position.X), to_float(position.Y)), std::format("Timer: {}s", (int)time_to_explode).c_str(), timer_color);
					}
                }
			}

            if (!var->config.visuals.skills)
                return;

            for (const auto& actor : actors_data.skills_list) {
                auto relative_location = actor.skill.RelativeLocation();
                auto position = m_core_engine->ProjectWorldToScreen(relative_location);

                if (m_core_engine->inScreen(position)) {

                    auto color = IM_COL32(var->config.visuals.skills_color[0] * 255.f,
                        var->config.visuals.skills_color[1] * 255.f,
                        var->config.visuals.skills_color[2] * 255.f,
                        var->config.visuals.skills_color[3] * 255.f);

                    maxi::render::m_render->DrawPlayerName(ImVec2(to_float(position.X), to_float(position.Y)), actor.name.c_str(), color);
                }
            }
        }


        void ui_main_loop() {
            while (m_running) {
                if (m_paused) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                try {
                    bool gui_only = (wcsstr(GetCommandLineW(), L"--gui-only") != nullptr);
                    if (maxi::render::m_render) {


                        if (maxi::render::m_render->is_hook_active()) {
                            {
                                static bool s_last_ins = false;
                                const bool ins = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
                                if (ins && !s_last_ins) var->gui.enabled = !var->gui.enabled;
                                s_last_ins = ins;
                            }
                            publish_discord_frame();
                        } else {
                            maxi::render::m_render->begin();

                            if (false && var->config.combat.draw_fov) {
                                maxi::render::m_render->DrawCircle(
                                    ImVec2(maxi::render::m_render->Width / 2, maxi::render::m_render->Height / 2),
                                    var->config.combat.fov,
                                    IM_COL32(255, 255, 255, 255),
                                    30,
                                    1.0f
                                );
                            }




                            if (!var->gui.enabled) {


                                ReleaseCapture();
                                draw_players();
                                draw_fov_circle();
                                draw_radar();
                            }
                            extern ULONGLONG g_prof[8];
                            g_prof[2] = GetTickCount64();
                            if (!var->gui.enabled) {



                                draw_timing_bar();
                            }
                            g_prof[3] = GetTickCount64();


                            publish_discord_frame();

                            maxi::render::m_render->end();
                        }
                    }
                }
                catch (const std::exception& e) {
                    LOG_ERROR(std::format("Exception in UI loop: {}", e.what()));
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (m_unload_pending) {
                    m_unload_pending = false;
                    teardown_for_exit();
                    LOG_WARNING("[UNLOAD] closing process...");
                    ExitProcess(0);
                }

                {
                    auto qpc_ms = []() -> double {
                        static LARGE_INTEGER f = {};
                        if (!f.QuadPart) QueryPerformanceFrequency(&f);
                        LARGE_INTEGER c{};
                        QueryPerformanceCounter(&c);
                        return 1000.0 * (double)c.QuadPart / (double)(f.QuadPart ? f.QuadPart : 1);
                    };
                    static double s_fprev = 0.0;
                    static double s_fsum = 0.0, s_fmax = 0.0;
                    static uint64_t s_fn = 0, s_f25 = 0, s_f50 = 0;
                    const double ft = qpc_ms();
                    if (s_fprev > 0.0) {
                        const double d = ft - s_fprev;
                        s_fsum += d; if (d > s_fmax) s_fmax = d;
                        if (d > 25.0) ++s_f25;
                        if (d > 50.0) ++s_f50;
                        if (++s_fn >= 600) {
                            LOG_INFO(std::format("[FRAME] avg={:.1f}ms max={:.0f}ms over25={} over50={} (600 frame)",
                                s_fsum / (double)s_fn, s_fmax, s_f25, s_f50));
                            s_fn = 0; s_fsum = 0.0; s_fmax = 0.0; s_f25 = 0; s_f50 = 0;
                        }
                    }
                    s_fprev = ft;
                }




                {
                    auto qpc_ms = []() -> double {
                        static LARGE_INTEGER f = {};
                        if (!f.QuadPart) QueryPerformanceFrequency(&f);
                        LARGE_INTEGER c{};
                        QueryPerformanceCounter(&c);
                        return 1000.0 * (double)c.QuadPart / (double)(f.QuadPart ? f.QuadPart : 1);
                    };
                    static double s_pf = 0.0;
                    static double s_sl = 3.0;
                    static int s_hz = 0;
                    static double s_hzchk = 0.0;
                    const double now = qpc_ms();
                    if (!s_hz || now - s_hzchk > 10000.0) {
                        s_hzchk = now;
                        DEVMODE dm{};
                        dm.dmSize = sizeof(dm);
                        if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency >= 60)
                            s_hz = (int)dm.dmDisplayFrequency;
                        if (!s_hz) s_hz = 60;



                        if (s_hz > 240) s_hz = 240;
                    }
                    const double budget = 1000.0 / (double)s_hz;
                    double slp = 1.0;
                    if (s_pf > 0.0) {
                        const double period = now - s_pf;
                        const double work = period - s_sl;
                        slp = budget - work;
                        if (slp < 0.0) slp = 0.0;
                        if (slp > budget) slp = budget;
                    }
                    s_sl = slp;
                    s_pf = now;
                    if (s_sl >= 0.5) Sleep((DWORD)(s_sl + 0.5));
                }
            }
        }

        void monitor_loop() {

            int tick = 0;
            int misses = 0;
            while (m_running) {
                try {
                    if (++tick >= 2000) {
                        tick = 0;
                        const uint32_t pid = (maxi::driver::g_driver)
                            ? maxi::driver::g_driver->get_process_id() : 0;
                        if (pid == 0) {
                            misses = 0;
                        } else {
                            bool alive = false;
                            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                            if (h) {
                                DWORD ec = 0;
                                if (GetExitCodeProcess(h, &ec)) alive = (ec == STILL_ACTIVE);
                                else alive = true;
                                CloseHandle(h);
                            } else {
                                alive = (GetLastError() == ERROR_ACCESS_DENIED);
                            }
                            if (alive) misses = 0;
                            else if (++misses >= 3) {
                                LOG_WARNING("[WATCH] game process closed - starting full unload...");
                                full_unload();
                                break;
                            }
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Exception in monitor loop: " + std::string(e.what()));
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    };


    std::shared_ptr<IApplication> create_application() {
        return std::make_shared<Application>();
    }

}
