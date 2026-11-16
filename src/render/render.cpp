#include <include/render/render.hpp>
#include <include/driver/driver_context.hpp>
#include <include/utils/logger.hpp>
#include <include/discord_hook/injector.hpp>
#include <include/discord_overlay/overlay_server.hpp>
#include <filesystem>
#include <tlhelp32.h>

#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dwmapi.lib")



extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ULONGLONG g_prof[8] = {};
static uint64_t g_frame_counter = 0;

namespace maxi::render {
    using namespace maxi::utils;

    namespace {
        inline std::wstring rnd_str(const uint16_t* enc, size_t n, uint16_t k) {
            std::wstring s(n, L'\0');
            for (size_t i = 0; i < n; ++i) s[i] = static_cast<wchar_t>(enc[i] ^ k);
            return s;
        }
        inline void set_overlay_src(char* dst, size_t cap, const uint8_t* enc, size_t n, uint8_t k) {
            for (size_t i = 0; i < n && i + 1 < cap; ++i)
                dst[i] = static_cast<char>(enc[i] ^ k);
            dst[n < cap ? n : cap - 1] = '\0';
        }
        inline void set_overlay_src_literal(char* dst, size_t cap, const char* src) {
            size_t n = strlen(src);
            for (size_t i = 0; i < n && i + 1 < cap; ++i) dst[i] = src[i];
            dst[n < cap ? n : cap - 1] = '\0';
        }


        inline void ov_external_discord(char* d, size_t c) { set_overlay_src_literal(d,c,"ExternalDiscordHWND"); }

        inline void ov_discord(char* d, size_t c)   { ov_external_discord(d,c); }
        inline void ov_crosshair(char* d, size_t c){ static const uint8_t e[]={0x2C,0x1D,0x00,0x1C,0x1C,0x07,0x0E,0x06,0x1D}; set_overlay_src(d,c,e,9,0x6F); }
        inline void ov_medal(char* d, size_t c)     { static const uint8_t e[]={0x64,0x4C,0x4D,0x48,0x45}; set_overlay_src(d,c,e,5,0x29); }
        inline void ov_geforce(char* d, size_t c)   { static const uint8_t e[]={0x6B,0x49,0x6A,0x43,0x5E,0x4F,0x49}; set_overlay_src(d,c,e,7,0x2C); }
        inline void ov_own(char* d, size_t c)       { static const uint8_t e[]={0x39,0x01,0x18,0x39,0x00,0x13,0x04,0x1A,0x17,0x0F}; set_overlay_src(d,c,e,10,0x76); }
    }

    namespace {
        struct s_render_enum_ctx {
            DWORD pid;
            HWND  hwnd;
        };

        BOOL CALLBACK render_enum_window_by_pid_proc(HWND hwnd, LPARAM lparam) {
            auto* ctx = reinterpret_cast<s_render_enum_ctx*>(lparam);
            DWORD wnd_pid = 0;
            GetWindowThreadProcessId(hwnd, &wnd_pid);
            if (wnd_pid == ctx->pid && IsWindowVisible(hwnd)) {
                ctx->hwnd = hwnd;
                return FALSE;
            }
            return TRUE;
        }

        HWND render_find_window_by_pid(DWORD pid) {
            if (!pid) return nullptr;
            s_render_enum_ctx ctx{ pid, nullptr };
            EnumWindows(render_enum_window_by_pid_proc, reinterpret_cast<LPARAM>(&ctx));
            return ctx.hwnd;
        }
    }

    LRESULT CALLBACK RENDER::standalone_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {


        if (msg == WM_NCHITTEST) return HTTRANSPARENT;
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }






    bool RENDER::reacquire_target_window(bool discord_only) {
        HWND cand = nullptr;



        cand = FindWindowA(xorstr_("DiscordDesktopOverlayInputTrap"), 0);
        if (!cand) cand = FindWindowA(0, xorstr_("Discord Overlay Input Trap"));
        if (!cand) cand = FindWindowA(xorstr_("Chrome_WidgetWin_1"), xorstr_("Discord Overlay"));
        if (!cand) cand = FindWindowA(0, xorstr_("Discord Overlay"));
        if (!cand) cand = FindWindowA(0, xorstr_("discord overlay"));
        if (!cand) cand = FindWindowA(0, xorstr_("Discord Streamkit Overlay"));
        if (!cand) cand = FindWindowA(xorstr_("Chrome_WidgetWin_1"), xorstr_("discord overlay"));
        if (cand) {
            ov_external_discord(m_overlay_source, sizeof(m_overlay_source));
            m_last_hijack_hwnd = cand;

            return true;
        }


        {
            static bool s_disc_diag = false;
            if (!s_disc_diag) {
                s_disc_diag = true;
                struct EnumCtx {
                    std::vector<std::wstring> cls;
                    std::vector<std::wstring> ttl;
                } ctx;
                EnumWindows([](HWND h, LPARAM lp) -> BOOL {
                    auto* c = reinterpret_cast<EnumCtx*>(lp);
                    wchar_t cls[256]{}, ttl[256]{};
                    if (GetClassNameW(h, cls, 256) && GetWindowTextW(h, ttl, 256)) {
                        std::wstring cw(cls), tw(ttl);
                        auto low = cw; std::transform(low.begin(), low.end(), low.begin(), ::towlower);
                        auto low2 = tw; std::transform(low2.begin(), low2.end(), low2.begin(), ::towlower);
                        if (low.find(L"chrome") != std::wstring::npos ||
                            low.find(L"electron") != std::wstring::npos ||
                            low.find(L"discord") != std::wstring::npos ||
                            low2.find(L"discord") != std::wstring::npos ||
                            low2.find(L"overlay") != std::wstring::npos) {
                            c->cls.push_back(cw); c->ttl.push_back(tw);
                        }
                    }
                    return TRUE;
                }, reinterpret_cast<LPARAM>(&ctx));
                std::string diag;
                for (size_t i = 0; i < ctx.cls.size(); ++i) {
                    std::string cs(ctx.cls[i].begin(), ctx.cls[i].end());
                    std::string ts(ctx.ttl[i].begin(), ctx.ttl[i].end());
                    diag += " [cls='" + cs + "' ttl='" + ts + "']";
                }
                LOG_INFO(std::format("[DISCORD DIAG] overlay not found. Discord-like windows:{}", diag));



                std::vector<DWORD> dpids;
                std::string procs;
                HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (snap != INVALID_HANDLE_VALUE) {
                    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
                    if (Process32FirstW(snap, &pe)) {
                        do {
                            std::wstring ex(pe.szExeFile);
                            std::wstring low = ex; std::transform(low.begin(), low.end(), low.begin(), ::towlower);
                            if (low.find(L"discord") != std::wstring::npos || low == L"update.exe") {
                                dpids.push_back(pe.th32ProcessID);
                                procs += " " + std::string(ex.begin(), ex.end()) + "(" + std::to_string(pe.th32ProcessID) + ")";
                            }
                        } while (Process32NextW(snap, &pe));
                    }
                    CloseHandle(snap);
                }
                LOG_INFO(std::format("[DISCORD DIAG2] discord processes:{}", procs.empty() ? "(none)" : procs));
                if (!dpids.empty()) {
                    struct Ctx2 { std::vector<DWORD> pids; std::string out; };
                    Ctx2 ctx2{ dpids, "" };
                    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
                        auto* c = reinterpret_cast<Ctx2*>(lp);
                        DWORD wpid = 0; GetWindowThreadProcessId(h, &wpid);
                        bool mine = false;
                        for (DWORD p : c->pids) if (p == wpid) { mine = true; break; }
                        if (!mine) return TRUE;
                        wchar_t cls[256]{}, ttl[256]{};
                        GetClassNameW(h, cls, 256); GetWindowTextW(h, ttl, 256);
                        std::string cs(cls, cls + wcslen(cls)), ts(ttl, ttl + wcslen(ttl));
                        if (cs.find("AMDDVR") != std::string::npos) return TRUE;
                        c->out += " [pid=" + std::to_string(wpid) + " cls='" + cs + "' ttl='" + ts + "']";
                        return TRUE;
                    }, reinterpret_cast<LPARAM>(&ctx2));
                    LOG_INFO(std::format("[DISCORD DIAG2] windows:{}", ctx2.out.empty() ? "(none)" : ctx2.out));
                }
            }
        }

        if (discord_only) return false;

        cand = FindWindowA(0, xorstr_("CrosshairX"));
        if (!cand) cand = FindWindowA(xorstr_("CrosshairX"), 0);
        if (!cand) cand = FindWindowA(0, xorstr_("Crosshair"));
        if (!cand) cand = FindWindowA(xorstr_("Crosshair_Class"), 0);
        if (cand) {
            ov_crosshair(m_overlay_source, sizeof(m_overlay_source));
            m_last_hijack_hwnd = cand;
            return true;
        }

        cand = FindWindowA(0, xorstr_("Medal Overlay"));
        if (!cand) cand = FindWindowA(xorstr_("MedalOverlayClass"), 0);
        if (!cand) cand = FindWindowA(xorstr_("MedalEncoder"), 0);
        if (!cand) cand = FindWindowA(xorstr_("MedalOverlay"), 0);
        if (!cand) cand = FindWindowA(xorstr_("Qt5152QWindowIcon"), xorstr_("Medal"));
        if (!cand) cand = FindWindowA(xorstr_("Qt5QWindowIcon"), xorstr_("Medal"));
        if (cand) {
            ov_medal(m_overlay_source, sizeof(m_overlay_source));
            m_last_hijack_hwnd = cand;
            return true;
        }

        cand = FindWindowA(xorstr_("CEF-OSC-WIDGET"), xorstr_("NVIDIA GeForce Overlay"));
        if (!cand) cand = FindWindowA(xorstr_("CEF-OSC-WINDOW"), nullptr);
        if (cand) {
            ov_geforce(m_overlay_source, sizeof(m_overlay_source));
            m_last_hijack_hwnd = cand;
            return true;
        }

        return false;
    }




    void RENDER::watchdog_tick() {



        bool alive = (m_last_hijack_hwnd && IsWindow(m_last_hijack_hwnd));
        if (alive && !m_standalone) {

            char tmp[32]; ov_external_discord(tmp, sizeof(tmp));
            if (_strnicmp(m_overlay_source, tmp, (int)strlen(tmp)) != 0) {
                alive = IsWindowVisible(m_last_hijack_hwnd);
            }
        }

        if (alive && m_overlay_active) {

            return;
        }

        if (!alive && m_overlay_active) {

            LOG_WARNING(std::format("[watchdog] HWND liveness lost ({}), re-discovering (renderer unaffected)...", m_overlay_source));
            m_overlay_active = false;

            if (!m_rendering_active) hWnd = nullptr;
            m_watchdog_last_lost_ms = GetTickCount64();
        }




        if (!m_overlay_active &&
            (GetTickCount64() - m_watchdog_last_lost_ms) >= 700 &&
            reacquire_target_window(/*discord_only=*/true)) {

            if (m_own_overlay) ov_own(m_overlay_source, sizeof(m_overlay_source));
            LOG_INFO(std::format("[watchdog] re-discovered external HWND: '{}' (no rendering hook - still on OwnOverlay)", m_overlay_source));
            m_overlay_active = true;


        }
    }


    RENDER::~RENDER() {
		shutdown();
	}

	bool RENDER::m_imgui_initialized = false;

	void RENDER::create_target_view() {
		ID3D11Texture2D* back_buffer {};
		if (swap_chain && SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
			device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
			back_buffer->Release();
		}
	}

	void RENDER::cleanup_target_view() {
        if (device_context) {
            device_context->OMSetRenderTargets(0, nullptr, nullptr);
        }
        if (render_target_view) { render_target_view->Release(); render_target_view = nullptr; }
	}

	bool RENDER::init() {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        Width = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
        Height = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));


        LOG_INFO("Waiting for target process/window to launch...");
        HWND hValo = nullptr;
        bool valo_found = false;
        bool gui_only = (wcsstr(GetCommandLineW(), L"--gui-only") != nullptr);
        int valo_wait_seconds = 0;
        uint32_t pid = 0;
        while (!valo_found && valo_wait_seconds < 120) {

            if (gui_only) {
                valo_found = true;
                break;
            }

            if (maxi::driver::g_driver) {
                pid = maxi::driver::g_driver->get_process_id();
                if (pid == 0) {
                    maxi::driver::g_driver->attach_process(maxi::driver::target_process_name_w());
                    pid = maxi::driver::g_driver->get_process_id();
                }
                if (pid != 0) {
                    LOG_INFO(std::format("Target process detected! (PID: {})", pid));
                    valo_found = true;
                    break;
                }
            }


            hValo = render_find_window_by_pid(pid);
            if (hValo) {
                LOG_INFO("Target window handle detected!");
                valo_found = true;
                break;
            }

            if (valo_wait_seconds % 5 == 0 && valo_wait_seconds > 0) {
                LOG_INFO(std::format("Still waiting for target to launch... ({}s)", valo_wait_seconds));
            }
            Sleep(1000);
            valo_wait_seconds++;
        }

        if (!valo_found) {
            if (gui_only) {
                LOG_INFO("GUI-only: no target found, creating standalone preview window...");
                HINSTANCE hInst = GetModuleHandleW(nullptr);
                WNDCLASSEXW wc{};
                wc.cbSize = sizeof(wc);
                wc.style = CS_HREDRAW | CS_VREDRAW;
                wc.lpfnWndProc = standalone_wnd_proc;
                wc.hInstance = hInst;
                wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

                static const uint16_t cls_enc[] = { 0x5C,0x70,0x69,0x78,0x42,0x65,0x70,0x7F,0x75,0x70,0x7D,0x7E,0x7F,0x74 };
                std::wstring cls_name = rnd_str(cls_enc, 14, 0x11);
                wc.lpszClassName = cls_name.c_str();
                RegisterClassExW(&wc);

                hWnd = CreateWindowExW(0, cls_name.c_str(), cls_name.c_str(),
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
                    nullptr, nullptr, hInst, nullptr);
                if (!hWnd) {
                    LOG_ERROR("Failed to create standalone preview window.");
                    return false;
                }
                m_standalone = true;
                m_overlay_active = true;
                valo_found = true;
                ShowWindow(hWnd, SW_SHOW);
                UpdateWindow(hWnd);
                LOG_INFO("Standalone preview window created successfully.");
            } else {
                LOG_WARNING("Target process/window not detected within 120 seconds. Aborting overlay initialization.");
                return false;
            }
        }

        LOG_INFO("Target process active! Proceeding to overlay initialization...");




        HWND externalDiscordHwnd = nullptr;
        DWORD discord_pid = 0;
        bool discord_hook_injected = false;
        

        constexpr bool kDiscordHookInjectionEnabled = false;
        if (kDiscordHookInjectionEnabled && !gui_only) {
            for (int g = 0; g < 1; ++g) {
                if (reacquire_target_window(/*discord_only=*/true)) {
                    externalDiscordHwnd = m_last_hijack_hwnd;

                    m_overlay_active = true;
                    LOG_INFO(std::format("[overlay-state] Discord HWND discovered: '{}' hwnd=0x{:X}", m_overlay_source, (uintptr_t)externalDiscordHwnd));
                    

                    GetWindowThreadProcessId(externalDiscordHwnd, &discord_pid);
                    if (discord_pid) {
                        LOG_INFO(std::format("[overlay-state] Discord PID: {}", discord_pid));
                        

                        if (maxi::discord_hook::Injector::CreateSharedMemory()) {
                            LOG_INFO("[overlay-state] Shared memory created for Discord hook");
                            

                            wchar_t exe_path[MAX_PATH];
                            GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
                            std::filesystem::path dll_path = std::filesystem::path(exe_path).parent_path() / L"DiscordHookDll.dll";
                            
                            if (std::filesystem::exists(dll_path)) {
                                LOG_INFO(std::format("[overlay-state] Injecting DiscordHookDll.dll into PID {}", discord_pid));
                                if (maxi::discord_hook::Injector::InjectDiscordHook(discord_pid, dll_path.wstring())) {
                                    discord_hook_injected = true;
                                    LOG_INFO("[overlay-state] Discord hook injection SUCCESS - rendering via Discord swapchain");

                                    m_overlay_source[0] = '\0';
                                    set_overlay_src_literal(m_overlay_source, sizeof(m_overlay_source), "HookedDiscordSwapChain");
                                } else {
                                    LOG_WARNING("[overlay-state] Discord hook injection FAILED - falling back to own overlay");
                                }
                            } else {
                                char dll_path_a[MAX_PATH] = {};
                                WideCharToMultiByte(CP_UTF8, 0, dll_path.c_str(), -1, dll_path_a, MAX_PATH, nullptr, nullptr);
                                LOG_WARNING(std::format("[overlay-state] DiscordHookDll.dll not found at: {}", dll_path_a));
                            }
                        }
                    }
                    break;
                }
                if (g == 0) {
                    LOG_INFO("Waiting for overlay host window (3s grace)...");
                }
                Sleep(1000);
            }
            if (externalDiscordHwnd && !discord_hook_injected) {
                LOG_INFO("[overlay-state] Discord HWND found but hook not injected - falling back to own overlay");
            }

            if (!externalDiscordHwnd) {
                LOG_INFO("[overlay-state] Discord HWND not found after 3s grace - trying Crosshair X fallback...");
                if (reacquire_target_window(/*discord_only=*/false)) {
                    LOG_INFO(std::format("[overlay-state] Fallback overlay host found: '{}' hwnd=0x{:X} (diagnostic only, rendering stays on own SwapChain)",
                        m_overlay_source, (uintptr_t)m_last_hijack_hwnd));
                    m_overlay_active = true;
                } else {
                    LOG_INFO("[overlay-state] No fallback overlay host found - using own overlay only");
                }
            }
        }


        if (discord_hook_injected) {
            Width = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
            Height = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
            m_rendering_active = false;
            m_swapchain_owned = false;
            LOG_INFO("[overlay-state] HookedDiscordSwapChain active - init OK without owned window (rendering in Discord)");
            return true;
        }


        if (false && maxi::discord_overlay::overlay_html_active()) {
            Width = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
            Height = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
            m_rendering_active = false;
            m_swapchain_owned = false;
            m_overlay_active = true;
            set_overlay_src_literal(m_overlay_source, sizeof(m_overlay_source), "DiscordOverlayHTML");
            LOG_INFO("[overlay-state] Discord overlay HTML active - OwnOverlay NOT opened (reads fast, realtime)");
            return true;
        }

        if (!discord_hook_injected && !hWnd && !gui_only) {            HINSTANCE hInst = GetModuleHandleW(nullptr);
            static bool s_own_class_done = false;
            if (!s_own_class_done) {
                WNDCLASSEXW wc{};
                wc.cbSize = sizeof(wc);
                wc.style = CS_HREDRAW | CS_VREDRAW;
                wc.lpfnWndProc = standalone_wnd_proc;
                wc.hInstance = hInst;
                wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

                static const uint16_t oo[] = { 0x6F,0x43,0x5A,0x4B,0x6D,0x55,0x4C,0x6D,0x54,0x47,0x50,0x4E,0x43,0x5B };
                std::wstring oo_name;
                for (int i = 0; i < 14; ++i) oo_name += static_cast<wchar_t>(oo[i] ^ 0x22);
                wc.lpszClassName = oo_name.c_str();
                RegisterClassExW(&wc);
                s_own_class_done = true;
            }
            const int sw = GetSystemMetrics(SM_CXSCREEN);
            const int sh = GetSystemMetrics(SM_CYSCREEN);
            static const uint16_t oo2[] = { 0x6F,0x43,0x5A,0x4B,0x6D,0x55,0x4C,0x6D,0x54,0x47,0x50,0x4E,0x43,0x5B };
            std::wstring oo_name2;
            for (int i = 0; i < 14; ++i) oo_name2 += static_cast<wchar_t>(oo2[i] ^ 0x22);
            hWnd = CreateWindowExW(
                WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                oo_name2.c_str(), L"", WS_POPUP,
                0, 0, sw, sh, nullptr, nullptr, hInst, nullptr);
            if (!hWnd) {
                LOG_ERROR("Own overlay window creation FAILED (err=" + std::to_string(GetLastError()) + ")");
                return false;
            }


            SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
            m_use_dcomp = true;
            ShowWindow(hWnd, SW_SHOWNOACTIVATE);
            UpdateWindow(hWnd);
            m_standalone = true;
            m_own_overlay = true;
            m_overlay_active = true;
            ov_own(m_overlay_source, sizeof(m_overlay_source));
            LOG_INFO(std::format("Own overlay fallback window created: {}x{} (host not found)", sw, sh));
        }

        if (!hWnd && gui_only) {
                LOG_INFO("GUI-only: creating standalone preview window...");
                HINSTANCE hInst = GetModuleHandleW(nullptr);
                WNDCLASSEXW wc{};
                wc.cbSize = sizeof(wc);
                wc.style = CS_HREDRAW | CS_VREDRAW;
                wc.lpfnWndProc = standalone_wnd_proc;
                wc.hInstance = hInst;
                wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
                static const uint16_t cls_enc2[] = { 0x5C,0x70,0x69,0x78,0x42,0x65,0x70,0x7F,0x75,0x70,0x7D,0x7E,0x7F,0x74 };
                std::wstring cls_name2 = rnd_str(cls_enc2, 14, 0x11);
                wc.lpszClassName = cls_name2.c_str();
                RegisterClassExW(&wc);

                hWnd = CreateWindowExW(0, cls_name2.c_str(), cls_name2.c_str(),
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
                    nullptr, nullptr, hInst, nullptr);
                if (!hWnd) {
                    LOG_ERROR("Failed to create standalone preview window.");
                    return false;
                }
                m_standalone = true;
                m_overlay_active = true;
                {
                    int sw = GetSystemMetrics(SM_CXSCREEN);
                    int sh = GetSystemMetrics(SM_CYSCREEN);
                    int w = static_cast<int>(sw * 0.80f);
                    int h = static_cast<int>(sh * 0.80f);
                    if (w > 1400) w = 1400;
                    if (h > 900) h = 900;
                    int x = (sw - w) / 2;
                    int y = (sh - h) / 2;
                    MoveWindow(hWnd, x, y, w, h, TRUE);
                }
                ShowWindow(hWnd, SW_SHOW);
                UpdateWindow(hWnd);
                LOG_INFO("Standalone preview window created successfully.");
        }

        if (!hWnd) {
            LOG_ERROR("Overlay window init failed (no host, no own window).");
            return false;
        }



        {
            DWORD wndPid = 0; GetWindowThreadProcessId(hWnd, &wndPid);
            LOG_INFO(std::format("[overlay-state] Own overlay selected: hwnd=0x{:X} pid={} (this pid={})", (uintptr_t)hWnd, wndPid, GetCurrentProcessId()));

            if (wndPid != GetCurrentProcessId()) {
                LOG_ERROR(std::format("[overlay-state] hWnd belongs to foreign pid {} - refusing to create SwapChain (would be second SwapChain)", wndPid));
                return false;
            }
        }

        char title_buf[256] = {};
        GetWindowTextA(hWnd, title_buf, sizeof(title_buf));
        LOG_INFO(std::format("[overlay-state] Overlay window ready (Source: {}, Title: '{}')", m_overlay_source, title_buf));



        if (!m_standalone) {
            SetMenu(hWnd, 0);
            SetWindowLongA(hWnd, GWL_EXSTYLE, static_cast<LONG_PTR>(GetWindowLongA(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED));
            MARGINS margin{ -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(hWnd, &margin);
            SetLayeredWindowAttributes(hWnd, 0x00, 254, LWA_ALPHA);
            MoveWindow(hWnd, 0, 0, static_cast<int>(Width), static_cast<int>(Height), FALSE);
            UpdateWindow(hWnd);
            ShowWindow(hWnd, SW_SHOW);

            SetWindowDisplayAffinity(hWnd, var->config.visuals.streamer_mode ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
        }

        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);
        LOG_INFO("[overlay-state] Creating Own D3D11 device");

        if (m_standalone) {
            RECT rc{};
            GetClientRect(hWnd, &rc);
            Width = static_cast<float>(rc.right - rc.left);
            Height = static_cast<float>(rc.bottom - rc.top);
        }

        const UINT createDeviceFlags = 0;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
        HRESULT res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &device, &featureLevel, &device_context);
        if (FAILED(res)) {
            LOG_ERROR(std::format("D3D11CreateDevice failed HRESULT: 0x{:08X}", static_cast<uint32_t>(res)));
            m_swapchain_owned = false; m_rendering_active = false; return false;
        }

        IDXGIDevice* dxgi_dev = nullptr;
        if (SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_dev)) && dxgi_dev) {
            IDXGIAdapter* adapter = nullptr;
            dxgi_dev->GetAdapter(&adapter);
            IDXGIFactory2* factory = nullptr;
            if (adapter) adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&factory);

            DXGI_SWAP_CHAIN_DESC1 sd1{};
            sd1.Width = static_cast<UINT>(Width);
            sd1.Height = static_cast<UINT>(Height);
            sd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd1.SampleDesc.Count = 1;
            sd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd1.BufferCount = 2;
            sd1.Scaling = DXGI_SCALING_STRETCH;
            sd1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            sd1.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            IDXGISwapChain1* sc1 = nullptr;
            res = factory ? factory->CreateSwapChainForComposition(device, &sd1, nullptr, &sc1) : E_FAIL;
            if (adapter) adapter->Release();
            if (factory) factory->Release();

            if (SUCCEEDED(res) && sc1) {
                sc1->QueryInterface(__uuidof(IDXGISwapChain), (void**)&swap_chain);
                if (swap_chain &&
                    SUCCEEDED(DCompositionCreateDevice(dxgi_dev, __uuidof(IDCompositionDevice), (void**)&m_dcomp_device)) && m_dcomp_device &&
                    SUCCEEDED(m_dcomp_device->CreateTargetForHwnd(hWnd, TRUE, &m_dcomp_target)) && m_dcomp_target &&
                    SUCCEEDED(m_dcomp_device->CreateVisual(&m_dcomp_visual)) && m_dcomp_visual) {
                    m_dcomp_visual->SetContent(sc1);
                    m_dcomp_target->SetRoot(m_dcomp_visual);
                    m_dcomp_device->Commit();
                    LOG_INFO("[overlay-state] DirectComposition overlay created (no WS_EX_LAYERED)");
                } else {
                    res = E_FAIL;
                }
                sc1->Release();
            } else {
                res = E_FAIL;
            }
            dxgi_dev->Release();
        } else {
            res = E_FAIL;
        }

        if (FAILED(res)) {
            LOG_ERROR(std::format("DComp overlay init failed HRESULT: 0x{:08X}", static_cast<uint32_t>(res)));
            m_swapchain_owned = false;
            m_rendering_active = false;
            return false;
        }
        LOG_INFO(std::format("[overlay-state] Own D3D11 device created (featureLevel=0x{:X})", (uint32_t)featureLevel));
        LOG_INFO("[overlay-state] Own SwapChain created");

		var->device_dx11 = device;
		var->swap_chain = swap_chain;
		var->device_context = device_context;
        m_swapchain_owned = true;

        cleanup_target_view();

        if (m_swapchain_owned && swap_chain) {
            HRESULT rb = swap_chain->ResizeBuffers(0, static_cast<UINT>(Width), static_cast<UINT>(Height), DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(rb)) {
                LOG_WARNING(std::format("[overlay-state] ResizeBuffers on owned SwapChain failed: 0x{:08X} (non-fatal)", (uint32_t)rb));
            }
        }
        create_target_view();
        LOG_INFO("[overlay-state] RTV created - rendering active");
        m_rendering_active = true;
        struct ResourceData {
            void* ptr = nullptr;
            DWORD size = 0;
        };

        auto load_rcdata = [](int id) -> ResourceData {
            HRSRC hRes = FindResourceW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(id), RT_RCDATA);
            if (!hRes) return {};
            HGLOBAL hData = LoadResource(GetModuleHandleW(nullptr), hRes);
            if (!hData) return {};
            return { LockResource(hData), SizeofResource(GetModuleHandleW(nullptr), hRes) };
        };

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImFontConfig cfg;
        cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_Bitmap;
        cfg.FontDataOwnedByAtlas = false;

        io.Fonts->Clear();

        ResourceData r_semibold = load_rcdata(IDR_FONT_INTER_SEMIBOLD);
        ResourceData r_medium = load_rcdata(IDR_FONT_INTER_MEDIUM);
        ResourceData r_logotype = load_rcdata(IDR_FONT_LOGOTYPE);
        ResourceData r_sec_icons = load_rcdata(IDR_FONT_SECTION_ICONS);
        ResourceData r_icons = load_rcdata(IDR_FONT_ICONS);
        ResourceData r_child_icons = load_rcdata(IDR_FONT_CHILD_ICONS);
        ResourceData r_key_icons = load_rcdata(IDR_FONT_KEYBIND_ICONS);
        ResourceData r_col_icons = load_rcdata(IDR_FONT_COLOR_ICONS);

        if (r_semibold.ptr && r_medium.ptr && r_logotype.ptr) {
            var->font.inter[0] = io.Fonts->AddFontFromMemoryTTF(r_semibold.ptr, r_semibold.size, SCALE(12.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.inter[1] = io.Fonts->AddFontFromMemoryTTF(r_medium.ptr, r_medium.size, SCALE(12.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.inter[2] = io.Fonts->AddFontFromMemoryTTF(r_medium.ptr, r_medium.size, SCALE(10.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.inter[3] = io.Fonts->AddFontFromMemoryTTF(r_semibold.ptr, r_semibold.size, SCALE(13.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());

            var->font.icons[0] = io.Fonts->AddFontFromMemoryTTF(r_logotype.ptr, r_logotype.size, SCALE(30.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[1] = io.Fonts->AddFontFromMemoryTTF(r_sec_icons.ptr, r_sec_icons.size, SCALE(18.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[2] = io.Fonts->AddFontFromMemoryTTF(r_icons.ptr, r_icons.size, SCALE(10.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[3] = io.Fonts->AddFontFromMemoryTTF(r_child_icons.ptr, r_child_icons.size, SCALE(12.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[4] = io.Fonts->AddFontFromMemoryTTF(r_icons.ptr, r_icons.size, SCALE(8.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[5] = io.Fonts->AddFontFromMemoryTTF(r_icons.ptr, r_icons.size, SCALE(11.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[6] = io.Fonts->AddFontFromMemoryTTF(r_key_icons.ptr, r_key_icons.size, SCALE(10.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[7] = io.Fonts->AddFontFromMemoryTTF(r_key_icons.ptr, r_key_icons.size, SCALE(5.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[8] = io.Fonts->AddFontFromMemoryTTF(r_icons.ptr, r_icons.size, SCALE(15.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[9] = io.Fonts->AddFontFromMemoryTTF(r_col_icons.ptr, r_col_icons.size, SCALE(11.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
        }
        else {
            LOG_ERROR("Failed to load font resources!");
            return false;
        }

        io.Fonts->Build();

        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX11_Init(device, device_context);
        m_imgui_initialized = true;
		return true;
	}


	void RENDER::begin() {




        m_watchdog_tick++;
        if ((m_watchdog_tick % 50) == 0) {
            watchdog_tick();
        }
        if (!m_rendering_active) {

            return;
        }

        if (m_standalone) {
            RECT rc{};
            GetClientRect(hWnd, &rc);
            Width = static_cast<float>(rc.right - rc.left);
            Height = static_cast<float>(rc.bottom - rc.top);
        } else {
            Width = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
            Height = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
        }


        if (m_own_overlay && hWnd) {
            static int s_menu_prev = -1;
            const int menu_now = var->gui.enabled ? 1 : 0;
            if (menu_now != s_menu_prev) {
                s_menu_prev = menu_now;
                LONG_PTR ex = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
                if (menu_now) ex &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
                else          ex |= static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
                SetWindowLongPtrW(hWnd, GWL_EXSTYLE, ex);
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }

        static bool streamer_mode_set = var->config.visuals.streamer_mode;
        if (streamer_mode_set != var->config.visuals.streamer_mode) {
            streamer_mode_set = var->config.visuals.streamer_mode;
            SetWindowDisplayAffinity(hWnd, streamer_mode_set ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
        }


		MSG msg;
		while (::PeekMessageA(&msg, 0, 0U, 0U, 0x0001)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		g_prof[0] = GetTickCount64();
		UpdateKeyStates();
		UpdateMouseState();

ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        gui->render();
        
        g_prof[1] = GetTickCount64();
    }

    void RENDER::end() {
        bool discord_hook_active = (strcmp(m_overlay_source, "HookedDiscordSwapChain") == 0);
        if (discord_hook_active) {


            return;
        }
        
        if (!m_rendering_active || !m_swapchain_owned || !swap_chain) return;
        if (!render_target_view || !device_context) return;
        ImGui::Render();
        g_prof[4] = GetTickCount64();

        const float clear_color_with_alpha[4] = { 0.f, 0.f, 0.f, 0.f };
        device_context->OMSetRenderTargets(1, &render_target_view, NULL);
        device_context->ClearRenderTargetView(render_target_view, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_prof[5] = GetTickCount64();
        HRESULT hr = swap_chain->Present(0, 0);
        g_prof[6] = GetTickCount64();



        static ULONGLONG last_present_err_log = 0;
        static uint64_t present_fail_streak = 0;
        const ULONGLONG now = GetTickCount64();
        if (FAILED(hr)) {
            present_fail_streak++;
            if (now - last_present_err_log > 2000) {
                last_present_err_log = now;
                LOG_ERROR(std::format("[UI] Present FAILED HRESULT=0x{:08X} (streak={}) - swapchain lost/frozen?",
                    static_cast<uint32_t>(hr), present_fail_streak));
            }
        } else {
            present_fail_streak = 0;
        }

		static uint64_t ui_frames = 0;
		static ULONGLONG s_prev_frame_t = 0;
		static ULONGLONG s_slow_log_t = 0;
		ui_frames++;
		const ULONGLONG now_t = GetTickCount64();
		if (s_prev_frame_t && (now_t - s_prev_frame_t) > 100 && (now_t - s_slow_log_t) > 2000) {
			s_slow_log_t = now_t;
			extern ULONGLONG g_prof[8];
			LOG_INFO(std::format("[UI] slow frame: {}ms gap (frame={}) | gui={} players={} nav={} render={} drawdata={} present={} ms | owned={} rendering={}",
				now_t - s_prev_frame_t, ui_frames,
				g_prof[1] - g_prof[0], g_prof[2] - g_prof[1], g_prof[3] - g_prof[2],
				g_prof[4] - g_prof[3], g_prof[5] - g_prof[4], g_prof[6] - g_prof[5],
				m_swapchain_owned?1:0, m_rendering_active?1:0));
		}
		s_prev_frame_t = now_t;
		if ((ui_frames % 300) == 1) {
			LOG_INFO(std::format("[UPROF] gui={} players={} nav={} render={} drawdata={} present={} ms",
				g_prof[1] - g_prof[0], g_prof[2] - g_prof[1], g_prof[3] - g_prof[2],
				g_prof[4] - g_prof[3], g_prof[5] - g_prof[4], g_prof[6] - g_prof[5]));
			LOG_INFO(std::format("[UI] heartbeat frame={} gui_enabled={} overlay='{}' owned={} rendering={}",
				ui_frames, var->gui.enabled ? 1 : 0, m_overlay_source, m_swapchain_owned?1:0, m_rendering_active?1:0));
		}
    }

bool RENDER::is_hook_active() const {


    return (std::strcmp(m_overlay_source, "HookedDiscordSwapChain") == 0)
        || (std::strcmp(m_overlay_source, "DiscordOverlayHTML") == 0);
    }

	void RENDER::shutdown() {
		if (m_imgui_initialized) {
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			m_imgui_initialized = false;
		}
		if (device_context && render_target_view && swap_chain && m_swapchain_owned) {
			const float clear_color_with_alpha[4] = { 0.f, 0.f, 0.f, 0.f };
			device_context->OMSetRenderTargets(1, &render_target_view, NULL);
			device_context->ClearRenderTargetView(render_target_view, clear_color_with_alpha);
			swap_chain->Present(0, 0);
		}
		cleanup_target_view();
		if (swap_chain) {
			if (m_swapchain_owned) { swap_chain->Release(); }
			swap_chain = nullptr;
			m_swapchain_owned = false;
		}
		if (device_context) { device_context->Release(); device_context = nullptr; }
		if (device) { device->Release(); device = nullptr; }
		m_rendering_active = false;
		m_overlay_active = false;
	}

	float RENDER::get_fps() const {
		return ImGui::GetIO().Framerate;
	}

	void RENDER::UpdateKeyStates() {
		ImGuiIO& io = ImGui::GetIO();
		for (int i = 0; i < 0x100; ++i) {
            auto isPushed = IsKeyPushed(i);
            ImGuiKey key = vk_to_imguikey(i);
            if (key != ImGuiKey_None) {
                io.AddKeyEvent(key, isPushed);
            }
            if (i == VK_INSERT && isPushed)
                var->gui.enabled = !var->gui.enabled;
            if (i == VK_END && isPushed) {
                LOG_INFO("Unhook key pressed. Gracefully shutting down application...");
                PostQuitMessage(0);
            }
		}
	}

	void RENDER::UpdateMouseState() {
		ImGuiIO& io = ImGui::GetIO();

		POINT cursor_pos{};
		GetCursorPos(&cursor_pos);

		io.AddMousePosEvent(static_cast<float>(cursor_pos.x), static_cast<float>(cursor_pos.y));

		io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
		io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
		io.AddMouseButtonEvent(2, (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);
		io.AddMouseButtonEvent(3, (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0);
		io.AddMouseButtonEvent(4, (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0);
	}

}
