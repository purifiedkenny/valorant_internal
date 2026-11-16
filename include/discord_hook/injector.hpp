#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <include/driver/idriver.hpp>
#include "hook_shared.h"

namespace maxi::discord_hook {

struct Injector {


    static bool InjectDiscordHook(DWORD target_pid, const std::wstring& dll_path);


    static DWORD FindDiscordOverlayPid();


    static bool IsAlreadyInjected(DWORD pid);


    static bool EjectDiscordHook(DWORD pid);


    static bool CreateSharedMemory();


    static void WriteSharedFrameData(const void* frame_data, size_t size);
};

}