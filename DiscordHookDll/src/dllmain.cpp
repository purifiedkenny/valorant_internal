





#define IMGUI_DISABLE_FREETYPE

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <cstdio>
#include <mutex>
#include <atomic>
#include <format>
#include <string>
#include <algorithm>

#include "../../include/discord_hook/hook_shared.h"

struct ImFontBuilderIO;
namespace ImGuiFreeType {
    IMGUI_API const ImFontBuilderIO* GetBuilderForFreeType() { return nullptr; }
}

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


static void HookLog(const char* msg) {
    char path[MAX_PATH]{};
    DWORD n = GetTempPathA(MAX_PATH, path);
    if (!n) return;
    strcat_s(path, "discordhook_dbg.log");
    FILE* f = nullptr;
    if (fopen_s(&f, path, "a") == 0 && f) { fprintf(f, "%lu %s\n", GetTickCount(), msg); fclose(f); }
}

namespace {

    typedef HRESULT(STDMETHODCALLTYPE* PresentFn)(IDXGISwapChain*, UINT, UINT);


    std::mutex g_mutex;
    IDXGISwapChain* g_swapchain = nullptr;
    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
    ID3D11RenderTargetView* g_rtv = nullptr;
    PresentFn g_original_present = nullptr;
    uintptr_t* g_vtable = nullptr;
    bool g_vtable_patched = false;
    HWND g_target_hwnd = nullptr;
    WNDPROC g_original_wndproc = nullptr;
    bool g_hooked = false;
    bool g_imgui_initialized = false;
    std::atomic<uint64_t> g_frame_id{0};


    HANDLE g_shared_mem = nullptr;
    discord_hook::SharedFrameData* g_shared_data = nullptr;
    HANDLE g_sync_mutex = nullptr;


    ImFont* g_font = nullptr;


    bool InitSharedMemory() {
        g_sync_mutex = OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, discord_hook::SYNC_MUTEX_NAME);
        if (!g_sync_mutex) {
            g_sync_mutex = CreateMutexW(nullptr, FALSE, discord_hook::SYNC_MUTEX_NAME);
        }

        g_shared_mem = OpenFileMappingW(FILE_MAP_READ, FALSE, discord_hook::SHARED_MEMORY_NAME);
        if (!g_shared_mem) {
            return false;
        }

        g_shared_data = (discord_hook::SharedFrameData*)MapViewOfFile(g_shared_mem, FILE_MAP_READ, 0, 0, sizeof(discord_hook::SharedFrameData));
        return g_shared_data != nullptr;
    }

    void CleanupSharedMemory() {
        if (g_shared_data) {
            UnmapViewOfFile(g_shared_data);
            g_shared_data = nullptr;
        }
        if (g_shared_mem) {
            CloseHandle(g_shared_mem);
            g_shared_mem = nullptr;
        }
        if (g_sync_mutex) {
            CloseHandle(g_sync_mutex);
            g_sync_mutex = nullptr;
        }
    }


    bool InitImGui(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;


        g_font = io.Fonts->AddFontDefault();
        io.Fonts->Build();

        if (!ImGui_ImplWin32_Init(hwnd)) return false;
        if (!ImGui_ImplDX11_Init(device, context)) return false;

        g_imgui_initialized = true;
        return true;
    }

    void CleanupImGui() {
        if (g_imgui_initialized) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            g_imgui_initialized = false;
        }
    }


    void CreateRTV() {
        if (g_rtv) return;
        ID3D11Texture2D* back_buffer = nullptr;
        if (SUCCEEDED(g_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer))) {
            g_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
            back_buffer->Release();
        }
    }

    void CleanupRTV() {
        if (g_rtv) {
            g_rtv->Release();
            g_rtv = nullptr;
        }
    }


    LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (g_imgui_initialized && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;
        return CallWindowProcW(g_original_wndproc, hwnd, msg, wParam, lParam);
    }

    void HookWndProc(HWND hwnd) {
        if (hwnd && !g_original_wndproc) {
            g_target_hwnd = hwnd;
            g_original_wndproc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
        }
    }

    void UnhookWndProc() {
        if (g_target_hwnd && g_original_wndproc) {
            SetWindowLongPtrW(g_target_hwnd, GWLP_WNDPROC, (LONG_PTR)g_original_wndproc);
            g_original_wndproc = nullptr;
            g_target_hwnd = nullptr;
        }
    }


    void RenderESP() {
        if (!g_shared_data) return;


        if (g_sync_mutex) {
            DWORD wait = WaitForSingleObject(g_sync_mutex, 2);
            if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) return;
        }

        uint32_t count = g_shared_data->actor_count;
        if (count > 64) count = 64;

        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        if (!draw) {
            if (g_sync_mutex) ReleaseMutex(g_sync_mutex);
            return;
        }

        for (uint32_t i = 0; i < count; ++i) {
            const auto& actor = g_shared_data->actors[i];
            if (!actor.is_valid) continue;

            ImU32 color = actor.team_id == 2 ? IM_COL32(255, 50, 50, 255) : IM_COL32(50, 255, 100, 255);
            if (!actor.is_visible) color = IM_COL32(150, 150, 150, 200);


            if (g_shared_data->show_skeleton) {
                const float thickness = 1.5f;
                auto draw_line = [&](float x1, float y1, float x2, float y2) {
                    if (x1 > -100 && x1 < (float)g_shared_data->screen_width + 100 &&
                        y1 > -100 && y1 < (float)g_shared_data->screen_height + 100 &&
                        x2 > -100 && x2 < (float)g_shared_data->screen_width + 100 &&
                        y2 > -100 && y2 < (float)g_shared_data->screen_height + 100) {
                        draw->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
                    }
                };

                draw_line(actor.head_x, actor.head_y, actor.neck_x, actor.neck_y);
                draw_line(actor.neck_x, actor.neck_y, actor.chest_x, actor.chest_y);
                draw_line(actor.chest_x, actor.chest_y, actor.pelvis_x, actor.pelvis_y);
                draw_line(actor.chest_x, actor.chest_y, actor.l_shoulder_x, actor.l_shoulder_y);
                draw_line(actor.l_shoulder_x, actor.l_shoulder_y, actor.l_elbow_x, actor.l_elbow_y);
                draw_line(actor.l_elbow_x, actor.l_elbow_y, actor.l_wrist_x, actor.l_wrist_y);
                draw_line(actor.chest_x, actor.chest_y, actor.r_shoulder_x, actor.r_shoulder_y);
                draw_line(actor.r_shoulder_x, actor.r_shoulder_y, actor.r_elbow_x, actor.r_elbow_y);
                draw_line(actor.r_elbow_x, actor.r_elbow_y, actor.r_wrist_x, actor.r_wrist_y);
                draw_line(actor.pelvis_x, actor.pelvis_y, actor.l_hip_x, actor.l_hip_y);
                draw_line(actor.l_hip_x, actor.l_hip_y, actor.l_knee_x, actor.l_knee_y);
                draw_line(actor.l_knee_x, actor.l_knee_y, actor.l_ankle_x, actor.l_ankle_y);
                draw_line(actor.pelvis_x, actor.pelvis_y, actor.r_hip_x, actor.r_hip_y);
                draw_line(actor.r_hip_x, actor.r_hip_y, actor.r_knee_x, actor.r_knee_y);
                draw_line(actor.r_knee_x, actor.r_knee_y, actor.r_ankle_x, actor.r_ankle_y);
            }


            if (g_shared_data->show_boxes) {
                float head_y = actor.head_y;
                float pelvis_y = actor.pelvis_y;
                float center_x = (actor.l_shoulder_x + actor.r_shoulder_x) * 0.5f;
                float box_h = fabsf(pelvis_y - head_y);
                float box_w = box_h * 0.5f;
                float left = center_x - box_w * 0.5f;
                float right = center_x + box_w * 0.5f;
                float top = fminf(head_y, pelvis_y);
                float bottom = fmaxf(head_y, pelvis_y);

                draw->AddRect(ImVec2(left, top), ImVec2(right, bottom), color, 0.0f, 0, 1.5f);
            }


            if (g_shared_data->show_names && actor.agent_name[0]) {
                ImVec2 text_pos(actor.head_x, actor.head_y - 20.0f);
                ImVec2 text_size = g_font ? g_font->CalcTextSizeA(g_font->FontSize, FLT_MAX, 0.0f, actor.agent_name) : ImVec2(0,0);
                text_pos.x -= text_size.x * 0.5f;
                draw->AddText(g_font, g_font ? g_font->FontSize : 14.0f, text_pos, IM_COL32(255,255,255,255), actor.agent_name);
            }
        }

        if (g_sync_mutex) ReleaseMutex(g_sync_mutex);
    }


    void RenderMenu() {
        if (!g_shared_data || !g_shared_data->show_menu) return;

        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("MAXI Discord Overlay", &g_shared_data->show_menu)) {
            ImGui::Text("Frame: %llu", g_shared_data->frame_id);
            ImGui::Text("Actors: %d", g_shared_data->actor_count);
            ImGui::Separator();
            ImGui::Checkbox("Boxes", &g_shared_data->show_boxes);
            ImGui::Checkbox("Skeleton", &g_shared_data->show_skeleton);
            ImGui::Checkbox("Names", &g_shared_data->show_names);
            ImGui::Checkbox("Health", &g_shared_data->show_health);
            ImGui::SliderFloat("Scale", &g_shared_data->menu_scale, 0.5f, 2.0f);
            ImGui::Text("Injected: Discord Overlay Hook Active");
        }
        ImGui::End();
    }


    static uint64_t s_present_tick = 0;

    HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* sc, UINT sync_interval, UINT flags) {
        std::lock_guard<std::mutex> lock(g_mutex);



        if (!g_hooked || sc != g_swapchain) {
            CleanupRTV();
            if (g_context) { g_context->Release(); g_context = nullptr; }
            if (g_device) { g_device->Release(); g_device = nullptr; }

            g_swapchain = sc;
            if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_device)) || !g_device) {
                g_swapchain = nullptr;
                if (g_original_present) return g_original_present(sc, sync_interval, flags);
                return sc->Present(sync_interval, flags);
            }
            g_device->GetImmediateContext(&g_context);

            DXGI_SWAP_CHAIN_DESC desc{};
            HWND new_hwnd = nullptr;
            if (SUCCEEDED(sc->GetDesc(&desc))) new_hwnd = desc.OutputWindow;
            if (new_hwnd != g_target_hwnd) { UnhookWndProc(); HookWndProc(new_hwnd); }

            if (!InitSharedMemory()) {
                OutputDebugStringA("[DiscordHookDll] Shared memory not available\n");
            }

            CleanupImGui();
            if (!InitImGui(g_device, g_context, g_target_hwnd)) {
                OutputDebugStringA("[DiscordHookDll] ImGui init failed\n");
            }

            CreateRTV();
            g_hooked = true;
        }


        if ((++s_present_tick % 120) == 0) {
            CleanupRTV();
            CreateRTV();
        }


        if (g_imgui_initialized) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            RenderESP();
            RenderMenu();

            ImGui::Render();


            if (g_rtv && g_context) {
                g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
                float clear_color[4] = {0, 0, 0, 0};
                g_context->ClearRenderTargetView(g_rtv, clear_color);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            }
        }

        g_frame_id.fetch_add(1);

        {
            static bool s_present_ev = false;
            if (!s_present_ev) {
                s_present_ev = true;
                CreateEventW(nullptr, TRUE, TRUE, L"Global\\MAXI_DISCORD_PRESENT_OK");
                HookLog("HookedPresent: first Present - PRESENT_OK event set");
            }
        }

        {
            const uint64_t f = g_frame_id.load();
            if (f && (f % 300) == 0)
                HookLog(std::format("Present frames={} shared_actors={} imgui={}",
                    f, g_shared_data ? g_shared_data->actor_count : -1, g_imgui_initialized ? 1 : 0).c_str());
        }


        if (g_original_present) {
            return g_original_present(sc, sync_interval, flags);
        }
        return sc->Present(sync_interval, flags);
    }



    bool PatchVTable(IDXGISwapChain* any_sc, void* new_fn) {
        if (!any_sc) return false;
        uintptr_t* vtable = *(uintptr_t**)any_sc;
        DWORD old_protect;
        if (!VirtualProtect(&vtable[8], sizeof(void*), PAGE_READWRITE, &old_protect)) return false;
        g_original_present = (PresentFn)vtable[8];
        vtable[8] = (uintptr_t)new_fn;
        VirtualProtect(&vtable[8], sizeof(void*), old_protect, &old_protect);
        g_vtable = vtable;
        g_vtable_patched = true;
        return true;
    }

    bool UnpatchVTable() {
        if (!g_vtable || !g_original_present) return false;
        DWORD old_protect;
        if (!VirtualProtect(&g_vtable[8], sizeof(void*), PAGE_READWRITE, &old_protect)) return false;
        g_vtable[8] = (uintptr_t)g_original_present;
        VirtualProtect(&g_vtable[8], sizeof(void*), old_protect, &old_protect);
        g_vtable = nullptr;
        g_vtable_patched = false;
        return true;
    }


    extern "C" __declspec(dllexport) int __stdcall HookInit(void* swapchain, void* device) {
        if (!swapchain || !device) return -1;
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_hooked) return 0;

        g_swapchain = (IDXGISwapChain*)swapchain;
        g_device = (ID3D11Device*)device;
        g_device->GetImmediateContext(&g_context);

        DXGI_SWAP_CHAIN_DESC desc;
        if (SUCCEEDED(g_swapchain->GetDesc(&desc))) {
            g_target_hwnd = desc.OutputWindow;
            HookWndProc(g_target_hwnd);
        }

        if (!InitSharedMemory()) {
            OutputDebugStringA("[DiscordHookDll] Shared memory not ready (will retry)\n");
        }

        if (!InitImGui(g_device, g_context, g_target_hwnd)) {
            return -2;
        }

        CreateRTV();

        if (!PatchVTable(g_swapchain, (void*)HookedPresent)) {
            return -3;
        }

        g_hooked = true;
        OutputDebugStringA("[DiscordHookDll] HookInit OK\n");
        return 0;
    }





    extern "C" __declspec(dllexport) int __stdcall HookStartup() {
        HookLog("HookStartup: enter");
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_hooked || g_vtable_patched) { HookLog("HookStartup: already hooked"); return 0; }

        WNDCLASSW wc{};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"MaxiDummySC";
        if (!RegisterClassW(&wc)) { HookLog("HookStartup: RegisterClassW FAIL"); return -10; }
        HookLog("HookStartup: class ok");
        HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_POPUP,
                                    0, 0, 2, 2, nullptr, nullptr, wc.hInstance, nullptr);
        if (!hwnd) { HookLog("HookStartup: CreateWindowExW FAIL"); UnregisterClassW(wc.lpszClassName, wc.hInstance); return -11; }
        HookLog("HookStartup: window ok");

        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        IDXGISwapChain* sc = nullptr;
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 2;
        sd.BufferDesc.Height = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        int rc = -12;
        HookLog("HookStartup: before D3D11CreateDeviceAndSwapChain");
        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                                    0, nullptr, 0, D3D11_SDK_VERSION,
                                                    &sd, &sc, &dev, nullptr, &ctx);
        if (SUCCEEDED(hr) && sc) {
            HookLog("HookStartup: device+swapchain ok");
            if (PatchVTable(sc, (void*)HookedPresent)) {
                rc = 0;
                HookLog("HookStartup: vtable patched OK");
                CreateEventW(nullptr, TRUE, TRUE, L"Global\\MAXI_DISCORD_HOOK_OK");
            }
            else HookLog("HookStartup: PatchVTable FAIL");
        } else {
            HookLog(std::format("HookStartup: D3D11CreateDeviceAndSwapChain FAILED hr=0x{:X}", (unsigned)hr).c_str());
        }
        if (ctx) ctx->Release();
        if (sc) sc->Release();
        if (dev) dev->Release();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        HookLog(std::format("HookStartup: exit rc={}", rc).c_str());
        return rc;
    }

    extern "C" __declspec(dllexport) int __stdcall HookShutdown() {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_hooked) return 0;

        UnpatchVTable();
        UnhookWndProc();
        CleanupImGui();
        CleanupRTV();
        CleanupSharedMemory();

        if (g_context) { g_context->Release(); g_context = nullptr; }
        if (g_device) { g_device->Release(); g_device = nullptr; }
        g_swapchain = nullptr;
        g_original_present = nullptr;
        g_hooked = false;

        OutputDebugStringA("[DiscordHookDll] HookShutdown OK\n");
        return 0;
    }

    extern "C" __declspec(dllexport) int __stdcall HookQuery(int* present_count) {
        if (present_count) *present_count = (int)g_frame_id.load();
        return g_hooked ? 1 : 0;
    }

}


static DWORD WINAPI HookStartupThread(LPVOID) {
    HookLog("HookStartupThread: start (loader lock released)");
    int rc = HookStartup();
    HookLog(std::format("HookStartupThread: HookStartup returned {}", rc).c_str());
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            HookLog("DLL_PROCESS_ATTACH");

            {
                wchar_t exe[MAX_PATH]{};
                if (GetModuleFileNameW(nullptr, exe, MAX_PATH)) {
                    std::wstring e = exe;
                    std::transform(e.begin(), e.end(), e.begin(), ::towlower);
                    bool is_discord = e.find(L"discord") != std::wstring::npos
                                   || e.find(L"update.exe") != std::wstring::npos;
                    if (!is_discord) {
                        HookLog("DllMain: not a Discord process - hook skipped");
                        break;
                    }
                    HookLog("DllMain: Discord process detected - installing hook");
                }
            }



            {
                HANDLE h = CreateThread(nullptr, 0, HookStartupThread, nullptr, 0, nullptr);
                if (h) CloseHandle(h);
                else HookLog("DllMain: CreateThread FAILED");
            }
            OutputDebugStringA("[DiscordHookDll] DLL_PROCESS_ATTACH\n");
            break;
        case DLL_PROCESS_DETACH:

            OutputDebugStringA("[DiscordHookDll] DLL_PROCESS_DETACH\n");
            break;
    }
    return TRUE;
}