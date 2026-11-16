#include <include/discord_hook/injector.hpp>
#include <include/utils/logger.hpp>
#include <include/driver/driver_context.hpp>
#include <algorithm>
#include <TlHelp32.h>
#include <Psapi.h>
#include <vector>
#include <cstring>

namespace maxi::discord_hook {

static HANDLE g_shared_mem = nullptr;
static void* g_shared_data = nullptr;
static HANDLE g_sync_mutex = nullptr;

static bool RemoteModuleMatchesLocal(HANDLE hProcess, uintptr_t remote_base, const std::wstring& dll_path) {
    auto read_remote = [&](uintptr_t addr, void* out, size_t n) {
        SIZE_T rd = 0;
        return ReadProcessMemory(hProcess, (LPCVOID)addr, out, n, &rd) && rd == n;
    };
    DWORD e_lfanew = 0;
    if (!read_remote(remote_base + 0x3C, &e_lfanew, sizeof(e_lfanew))) return false;
    if (e_lfanew > 0x1000) return false;
    DWORD r_stamp = 0, r_size = 0;
    if (!read_remote(remote_base + e_lfanew + 8, &r_stamp, sizeof(r_stamp))) return false;
    if (!read_remote(remote_base + e_lfanew + 56, &r_size, sizeof(r_size))) return false;

    HANDLE hFile = CreateFileW(dll_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD l_lfanew = 0;
    DWORD got = 0;
    bool ok = false;
    DWORD l_stamp = 0, l_size = 0;
    SetFilePointer(hFile, 0x3C, nullptr, FILE_BEGIN);
    if (ReadFile(hFile, &l_lfanew, sizeof(l_lfanew), &got, nullptr) && got == sizeof(l_lfanew) && l_lfanew <= 0x1000) {
        SetFilePointer(hFile, l_lfanew + 8, nullptr, FILE_BEGIN);
        if (ReadFile(hFile, &l_stamp, sizeof(l_stamp), &got, nullptr) && got == sizeof(l_stamp)) {
            SetFilePointer(hFile, l_lfanew + 56, nullptr, FILE_BEGIN);
            if (ReadFile(hFile, &l_size, sizeof(l_size), &got, nullptr) && got == sizeof(l_size))
                ok = true;
        }
    }
    CloseHandle(hFile);
    if (!ok) return false;
    LOG_INFO(std::format("[hook] surum karsilastirma: uzak stamp=0x{:X} size=0x{:X} / lokal stamp=0x{:X} size=0x{:X}",
        r_stamp, r_size, l_stamp, l_size));
    return (r_stamp == l_stamp) && (r_size == l_size);
}

bool Injector::CreateSharedMemory() {
    g_sync_mutex = CreateMutexW(nullptr, FALSE, L"Global\\MAXI_DISCORD_HOOK_MUTEX");
    if (!g_sync_mutex) {
        LOG_ERROR("Failed to create sync mutex");
        return false;
    }

    g_shared_mem = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(::discord_hook::SharedFrameData),
        L"Global\\MAXI_DISCORD_HOOK_SHARED"
    );
    if (!g_shared_mem) {
        LOG_ERROR("Failed to create shared memory");
        return false;
    }

    g_shared_data = MapViewOfFile(g_shared_mem, FILE_MAP_WRITE, 0, 0, sizeof(::discord_hook::SharedFrameData));
    if (!g_shared_data) {
        LOG_ERROR("Failed to map shared memory view");
        return false;
    }

    LOG_INFO("Discord hook shared memory created");
    return true;
}

void Injector::WriteSharedFrameData(const void* frame_data, size_t size) {
    if (!g_shared_data || !frame_data) return;
    
    if (g_sync_mutex) {
        DWORD wait = WaitForSingleObject(g_sync_mutex, 2);
        if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) return;
    }

    memcpy(g_shared_data, frame_data, size < sizeof(::discord_hook::SharedFrameData) ? size : sizeof(::discord_hook::SharedFrameData));

    if (g_sync_mutex) ReleaseMutex(g_sync_mutex);
}

DWORD Injector::FindDiscordOverlayPid() {

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {

            std::wstring exe_name(entry.szExeFile);
            std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::towlower);
            
            if (exe_name.find(L"discord") != std::wstring::npos) {

                DWORD pid = entry.th32ProcessID;
                

                struct EnumCtx { DWORD pid; HWND hwnd = nullptr; };
                EnumCtx ctx{pid, nullptr};
                
                EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
                    auto* c = reinterpret_cast<EnumCtx*>(lparam);
                    DWORD wpid = 0;
                    GetWindowThreadProcessId(hwnd, &wpid);
                    if (wpid != c->pid) return TRUE;
                    
                    wchar_t class_name[256]{}, title[256]{};
                    GetClassNameW(hwnd, class_name, 256);
                    GetWindowTextW(hwnd, title, 256);
                    
                    std::wstring cls(class_name), ttl(title);
                    std::transform(cls.begin(), cls.end(), cls.begin(), ::towlower);
                    std::transform(ttl.begin(), ttl.end(), ttl.begin(), ::towlower);
                    
                    if ((cls.find(L"chrome_widgetwin_1") != std::wstring::npos && 
                         (ttl.find(L"discord") != std::wstring::npos || ttl.find(L"overlay") != std::wstring::npos)) ||
                        (ttl.find(L"discord overlay") != std::wstring::npos)) {
                        c->hwnd = hwnd;
                        return FALSE;
                    }
                    return TRUE;
                }, reinterpret_cast<LPARAM>(&ctx));
                
                if (ctx.hwnd) {
                    CloseHandle(snapshot);
                    LOG_INFO(std::format("Found Discord overlay PID: {} HWND: 0x{:X}", pid, (uintptr_t)ctx.hwnd));
                    return pid;
                }
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

bool Injector::IsAlreadyInjected(DWORD pid) {
    if (pid == 0) return false;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    if (Module32FirstW(snap, &me)) {
        do {
            std::wstring mod(me.szModule);
            std::transform(mod.begin(), mod.end(), mod.begin(), ::towlower);
            if (mod.find(L"discordhookdll") != std::wstring::npos) { found = true; break; }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}



static uint32_t GetExportRva(const std::wstring& dll_path, const char* export_name) {
    HANDLE hFile = CreateFileW(dll_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    DWORD sz = GetFileSize(hFile, nullptr);
    std::vector<uint8_t> buf(sz);
    DWORD got = 0;
    bool rok = sz > 0x40 && ReadFile(hFile, buf.data(), sz, &got, nullptr) && got == sz;
    CloseHandle(hFile);
    if (!rok) return 0;
    auto u16 = [&](size_t o) -> uint16_t { return *(uint16_t*)(buf.data() + o); };
    auto u32 = [&](size_t o) -> uint32_t { return *(uint32_t*)(buf.data() + o); };
    if (u16(0) != 0x5A4D) return 0;
    DWORD e_lfanew = u32(0x3C);
    if (e_lfanew + 0x18 > buf.size() || u32(e_lfanew) != 0x00004550) return 0;
    size_t coff = e_lfanew + 4;
    size_t opt = coff + 20;
    if (opt + 0x70 > buf.size()) return 0;
    uint16_t magic = u16(opt);
    size_t ddir = (magic == 0x20B) ? opt + 112 : opt + 96;
    if (ddir + 8 > buf.size()) return 0;
    DWORD exp_rva = u32(ddir);
    if (!exp_rva) return 0;
    uint16_t nsec = u16(coff + 2);
    size_t secoff = opt + u16(coff + 16);
    auto rva_to_off = [&](DWORD rva) -> size_t {
        for (int i = 0; i < (int)nsec; ++i) {
            size_t s = secoff + (size_t)i * 40;
            if (s + 40 > buf.size()) break;
            DWORD va = u32(s + 12), vsz = u32(s + 8), praw = u32(s + 20), rsz = u32(s + 16);
            DWORD span = vsz ? vsz : rsz;
            if (rva >= va && rva < va + span) return praw + (rva - va);
        }
        return 0;
    };
    size_t eoff = rva_to_off(exp_rva);
    if (!eoff || eoff + 40 > buf.size()) return 0;
    DWORD num_names = u32(eoff + 24);
    DWORD funcs_rva = u32(eoff + 28);
    DWORD names_rva = u32(eoff + 32);
    DWORD ords_rva = u32(eoff + 36);
    size_t foff = rva_to_off(funcs_rva), noff = rva_to_off(names_rva), ooff = rva_to_off(ords_rva);
    if (!foff || !noff || !ooff) return 0;
    for (DWORD i = 0; i < num_names; ++i) {
        size_t ne = noff + (size_t)i * 4;
        if (ne + 4 > buf.size()) break;
        size_t nstr = rva_to_off(u32(ne));
        if (!nstr || nstr >= buf.size()) continue;
        if (strcmp((const char*)(buf.data() + nstr), export_name) == 0) {
            size_t oe = ooff + (size_t)i * 2;
            if (oe + 2 > buf.size()) return 0;
            uint16_t ord = *(uint16_t*)(buf.data() + oe);
            return u32(foff + (size_t)ord * 4);
        }
    }
    return 0;
}

bool Injector::InjectDiscordHook(DWORD target_pid, const std::wstring& dll_path) {
    if (target_pid == 0) {
        LOG_ERROR("Invalid target PID");
        return false;
    }


    uintptr_t remote_base = 0;
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, target_pid);
        if (snap != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W me{};
            me.dwSize = sizeof(me);
            if (Module32FirstW(snap, &me)) {
                do {
                    std::wstring mod(me.szModule);
                    std::transform(mod.begin(), mod.end(), mod.begin(), ::towlower);
                    if (mod.find(L"discordhookdll") != std::wstring::npos) {
                        remote_base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                        break;
                    }
                } while (Module32NextW(snap, &me));
            }
            CloseHandle(snap);
        }
    }


    if (!maxi::driver::g_driver) {
        LOG_ERROR("Driver not available for injection");
        return false;
    }


    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, target_pid
    );
    
    if (!hProcess) {
        LOG_ERROR(std::format("OpenProcess failed for PID {}: {}", target_pid, GetLastError()));
        return false;
    }

    if (remote_base && !RemoteModuleMatchesLocal(hProcess, remote_base, dll_path)) {
        LOG_WARNING("[hook] target DLL is an old version - will eject and reload fresh");
        CloseHandle(hProcess);
        EjectDiscordHook(target_pid);
        hProcess = OpenProcess(
            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
            FALSE, target_pid
        );
        if (!hProcess) {
            LOG_ERROR(std::format("OpenProcess (retry) failed for PID {}: {}", target_pid, GetLastError()));
            return false;
        }
        remote_base = 0;
    } else if (remote_base) {
        LOG_INFO(std::format("DiscordHookDll already resident in PID {} (hmod=0x{:X}), ensuring hook...", target_pid, remote_base));
    }

    LPVOID remote_path = nullptr;
    if (!remote_base) {

        size_t path_size = (dll_path.size() + 1) * sizeof(wchar_t);
        remote_path = VirtualAllocEx(hProcess, nullptr, path_size, 
                                     MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remote_path) {
            LOG_ERROR("VirtualAllocEx failed");
            CloseHandle(hProcess);
            return false;
        }


        if (!WriteProcessMemory(hProcess, remote_path, dll_path.c_str(), path_size, nullptr)) {
            LOG_ERROR("WriteProcessMemory failed");
            VirtualFreeEx(hProcess, remote_path, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }


        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID load_library_addr = GetProcAddress(hKernel32, "LoadLibraryW");
        if (!load_library_addr) {
            LOG_ERROR("GetProcAddress LoadLibraryW failed");
            VirtualFreeEx(hProcess, remote_path, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }


        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, 
                                            (LPTHREAD_START_ROUTINE)load_library_addr,
                                            remote_path, 0, nullptr);
        if (!hThread) {
            LOG_ERROR(std::format("CreateRemoteThread failed: {}", GetLastError()));
            VirtualFreeEx(hProcess, remote_path, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }


        WaitForSingleObject(hThread, 5000);
        
        DWORD exit_code = 0;
        GetExitCodeThread(hThread, &exit_code);
        CloseHandle(hThread);

        if (exit_code == 0) {
            LOG_ERROR("DiscordHookDll injection failed (LoadLibraryW returned NULL)");
            VirtualFreeEx(hProcess, remote_path, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }
        remote_base = static_cast<uintptr_t>(exit_code);
        LOG_INFO(std::format("DiscordHookDll loaded into PID {} (hmod=0x{:X}), starting hook...", target_pid, exit_code));
    }




    uint32_t rva = GetExportRva(dll_path, "HookStartup");
    if (!rva) {
        LOG_ERROR("HookStartup export RVA not found in DLL (PE parse)");
        VirtualFreeEx(hProcess, remote_path, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    LPVOID fpRemote = reinterpret_cast<LPVOID>(remote_base + rva); (void)fpRemote;
    VirtualFreeEx(hProcess, remote_path, 0, MEM_RELEASE);



    HANDLE hEv = nullptr;
    for (int i = 0; i < 60 && !hEv; ++i) {
        hEv = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\MAXI_DISCORD_HOOK_OK");
        if (!hEv) Sleep(100);
    }
    CloseHandle(hProcess);
    if (hEv) {
        const DWORD w = WaitForSingleObject(hEv, 5000);
        CloseHandle(hEv);
        if (w == WAIT_OBJECT_0) {
            LOG_INFO(std::format("Discord hook vtable patched in PID {} - verifying Present...", target_pid));


            HANDLE hP = nullptr;
            for (int i = 0; i < 30 && !hP; ++i) {
                hP = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\MAXI_DISCORD_PRESENT_OK");
                if (!hP) Sleep(100);
            }
            if (hP) {
                const DWORD wp = WaitForSingleObject(hP, 3000);
                CloseHandle(hP);
                if (wp == WAIT_OBJECT_0) {
                    LOG_INFO(std::format("Discord hook ACTIVE and rendering in PID {}", target_pid));
                    return true;
                }
            }
            LOG_WARNING("Discord Present not firing - falling back to OwnOverlay");
            return false;
        }
    }
    LOG_ERROR("Discord hook event not signaled (HookStartup failed or timed out)");
    return false;
}

bool Injector::EjectDiscordHook(DWORD pid) {
    if (pid == 0) return false;


    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid
    );
    
    if (!hProcess) return false;


    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID get_proc_addr = GetProcAddress(hKernel32, "GetProcAddress");
    LPVOID load_lib = GetProcAddress(hKernel32, "LoadLibraryW");
    


    LPVOID free_lib = GetProcAddress(hKernel32, "FreeLibrary");
    

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) {
            do {
                std::wstring mod_name(me.szModule);
                std::transform(mod_name.begin(), mod_name.end(), mod_name.begin(), ::towlower);
                if (mod_name.find(L"discordhookdll") != std::wstring::npos) {

                    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                                        (LPTHREAD_START_ROUTINE)free_lib,
                                                        me.modBaseAddr, 0, nullptr);
                    if (hThread) {
                        WaitForSingleObject(hThread, 3000);
                        CloseHandle(hThread);
                    }
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
    }

    CloseHandle(hProcess);
    return true;
}

}
