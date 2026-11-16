#include "include/common.hpp"
#include "include/maxi.hpp"
#include "include/core/build_stamp.hpp"
#include "src/decoder/CoreDecoderAdapter.hpp"
#include <windows.h>
#include <fstream>
#include <string>


static std::wstring g_log_path;

static void write_debug_log(const std::wstring& msg) {
    if (g_log_path.empty()) return;
    std::wofstream f(g_log_path, std::ios::app);
    if (f.is_open()) f << msg << L"\n";
}

static LONG WINAPI maxi_crash_filter(EXCEPTION_POINTERS* ep) {
    if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = ep->ExceptionRecord->ExceptionCode;
    void* addr = ep->ExceptionRecord->ExceptionAddress;

    HMODULE mod = nullptr;
    uintptr_t rva = 0;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)addr, &mod) && mod) {
        rva = (uintptr_t)addr - (uintptr_t)mod;
    }

    wchar_t buf[512];
    if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
        swprintf(buf, 512, L"[CRASH] AV %s addr=%p target=0x%llX module=%p rva=0x%llX tid=%lu",
            ep->ExceptionRecord->ExceptionInformation[0] ? L"WRITE" : L"READ",
            addr, (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1],
            mod, (unsigned long long)rva, GetCurrentThreadId());
    } else {
        swprintf(buf, 512, L"[CRASH] code=0x%08X addr=%p module=%p rva=0x%llX tid=%lu",
            code, addr, mod, (unsigned long long)rva, GetCurrentThreadId());
    }
    write_debug_log(buf);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void maxi_terminate_handler() {
    write_debug_log(L"[CRASH] std::terminate (uncaught C++ exception - inside thread)");
    abort();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {



    {
        wchar_t self_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, self_path, MAX_PATH);
        std::wstring self(self_path);
        std::wstring self_lower = self;
        std::transform(self_lower.begin(), self_lower.end(), self_lower.begin(), ::towlower);
        const bool known_name = self_lower.find(L"maxiwindowsxp") != std::wstring::npos ||
                                self_lower.find(L"maxiwin") != std::wstring::npos;
        if (known_name && self_lower.find(L".exe") != std::wstring::npos) {
            auto slash = self.find_last_of(L"\\/");
            std::wstring dir = (slash != std::wstring::npos) ? self.substr(0, slash + 1) : L"";
            static const wchar_t alph[] = L"ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
            std::wstring rnd;
            rnd.reserve(12);
            ULONGLONG seed = GetTickCount64() ^ static_cast<ULONGLONG>(GetCurrentProcessId());
            for (int i = 0; i < 12; ++i) {
                seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
                rnd += alph[(seed >> 33) % 32];
            }
            std::wstring new_path = dir + rnd + L".exe";
            if (CopyFileW(self.c_str(), new_path.c_str(), FALSE)) {
                STARTUPINFOW si{ sizeof(si) };
                PROCESS_INFORMATION pi{};
                if (CreateProcessW(new_path.c_str(), GetCommandLineW(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    DeleteFileW(self.c_str());
                    return 0;
                }
            }
        }
    }


    static const wchar_t mtx_enc[] = { 0x5E,0x6A,0x5E,0x5C,0x62,0x0F,0x5A,0x6E,0x6E,0x67,0x6B,0x5A,0x6E,0x60,0x5C,0x63,0x64,0x5C,0x6A,0x2E,0x48,0x6E,0x6A,0x6B,0x6A,0x5C,0x6D,0x5B,0x63,0x2E,0x2D,0x0F,0x50,0x6A,0x6E,0x64,0x6B,0x2E,0x49,0x7A,0x6A,0x62,0x63,0x2E,0x48,0x6B,0x67,0x60 };
    std::wstring mtx_name;
    for (int i = 0; i < 48; ++i) mtx_name += static_cast<wchar_t>(mtx_enc[i] ^ 0x2D);

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mtx_name.c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }


    maxi::build::anchor_polymorphic_data();


    AllocConsole();
    FILE* f_out = nullptr;
    FILE* f_err = nullptr;
    freopen_s(&f_out, "CONOUT$", "w", stdout);
    freopen_s(&f_err, "CONOUT$", "w", stderr);
    SetConsoleTitleW(L"Windows Host Process");


    wchar_t exe_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    std::wstring exe_dir(exe_path);
    auto last_slash = exe_dir.find_last_of(L"\\/");
    if (last_slash != std::wstring::npos) exe_dir = exe_dir.substr(0, last_slash);
#ifdef _DEBUG
    g_log_path = exe_dir + L"\\maxi_debug.log";
    { std::wofstream clr(g_log_path, std::ios::trunc); }
    write_debug_log(L"[STARTUP] launching...");
#else
    g_log_path.clear();
#endif
    wprintf(L"[!] debug log: %s\n", g_log_path.c_str());


#ifdef _DEBUG
    maxi::utils::initialize_logger(maxi::utils::LogLevel::DEBUG);
#else
    maxi::utils::initialize_logger(maxi::utils::LogLevel::FATAL);
#endif
    write_debug_log(L"[STARTUP] Logger initialized");

    SetUnhandledExceptionFilter(maxi_crash_filter);
    std::set_terminate(maxi_terminate_handler);
    write_debug_log(L"[STARTUP] Crash reporter installed");

    {
        write_debug_log(L"[INIT] Core decoder metadata load...");
        auto core_metadata = maxi::core::decoder::make_default_metadata();
        if (!maxi::core::decoder::initialize_with_metadata(core_metadata)) {
            LOG_ERROR("Core decoder initialization failed - aborting startup");
            write_debug_log(L"[FAIL] core_decoder::initialize_with_metadata returned false");
            MessageBoxW(nullptr, L"Failed: core_decoder initialization (metadata/validation).\n\nCheck maxi_debug.log next to exe for details.",
                        xorstr_(L"MAXI Init Error"), MB_OK | MB_ICONERROR);
            if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
            return -1;
        }
        if (!maxi::core::decoder::validate_core_decoders()) {
            LOG_ERROR("Core decoder validation failed - aborting startup");
            write_debug_log(L"[FAIL] core_decoder::validate_core_decoders returned false");
            MessageBoxW(nullptr, L"Failed: core decoder validation.\n\nCheck maxi_debug.log next to exe for details.",
                        xorstr_(L"MAXI Init Error"), MB_OK | MB_ICONERROR);
            if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
            return -1;
        }
        write_debug_log(L"[OK] Core decoders initialized and validated");
        printf("[MAXI] [OK] Core decoders configured from build metadata and validated\n");
    }

    try {
        write_debug_log(L"[INIT] Creating application...");
        printf("[MAXI] Creating application...\n");


        auto app = maxi::core::create_application();
        if (!app) {
            write_debug_log(L"[FAIL] create_application() returned null");
            MessageBoxW(nullptr, L"Failed: create_application() returned null",
                        xorstr_(L"MAXI Init Error"), MB_OK | MB_ICONERROR);
            if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
            return -1;
        }
        maxi::core::g_app = app;

        write_debug_log(L"[INIT] Initializing systems...");
        printf("[MAXI] Initializing systems...\n");


        if (!app->initialize()) {
            write_debug_log(L"[FAIL] app->initialize() returned false");
            MessageBoxW(nullptr, L"Failed: app->initialize() returned false\n\nCheck maxi_debug.log next to exe for details.",
                        xorstr_(L"MAXI Init Error"), MB_OK | MB_ICONERROR);
            return -1;
        }

        write_debug_log(L"[OK] Initialization complete - Kernel Driver & Overlay successfully injected");
        printf("[MAXI] [SUCCESS] All systems GO! Kernel Driver & Overlay active. Hiding console window for 100%% Streamproof privacy...\n");


        HWND hConsole = GetConsoleWindow();
        if (hConsole) {
            ShowWindow(hConsole, SW_HIDE);
        }
        FreeConsole();


        app->run();

        if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
        return 0;
    }
    catch (const std::exception& e) {
        std::string msg = std::string("Exception: ") + e.what();
        write_debug_log(std::wstring(msg.begin(), msg.end()));
        MessageBoxA(nullptr, msg.c_str(), xorstr_("MAXI Fatal Exception"), MB_OK | MB_ICONERROR);
        return -1;
    }
    catch (...) {
        write_debug_log(L"[FATAL] Unknown exception caught");
        MessageBoxW(nullptr, L"Unknown fatal exception. Check maxi_debug.log.",
                    xorstr_(L"MAXI Fatal"), MB_OK | MB_ICONERROR);
        return -1;
    }
}
