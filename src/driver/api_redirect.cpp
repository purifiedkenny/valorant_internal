#include "../../include/driver/api_redirect.hpp"
#include "../../include/utils/logger.hpp"
#include <intrin.h>





namespace maxi::driver::api {





    struct UNICODE_STRING_PEB {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    };

    struct LDR_DATA_TABLE_ENTRY_PEB {
        LIST_ENTRY InLoadOrderLinks;
        LIST_ENTRY InMemoryOrderLinks;
        LIST_ENTRY InInitializationOrderLinks;
        PVOID      DllBase;
        PVOID      EntryPoint;
        ULONG      SizeOfImage;
        ULONG      Pad;
        UNICODE_STRING_PEB FullDllName;
        UNICODE_STRING_PEB BaseDllName;
    };

    struct PEB_LDR_DATA_PEB {
        ULONG      Length;
        BOOLEAN    Initialized;
        BYTE       Pad[3];
        HANDLE     SsHandle;
        LIST_ENTRY InLoadOrderModuleList;
        LIST_ENTRY InMemoryOrderModuleList;
        LIST_ENTRY InInitializationOrderModuleList;
    };

    struct PEB_MINIMAL {
        BYTE       Reserved1[2];
        BYTE       BeingDebugged;
        BYTE       Reserved2[1];
        PVOID      Reserved3[2];
        PEB_LDR_DATA_PEB* Ldr;
    };




    uint32_t djb2_hash_runtime(const char* str) {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + static_cast<uint8_t>(*str);
            ++str;
        }
        return hash;
    }


    static uint32_t djb2_hash_wide_lower(const wchar_t* str, size_t len) {
        uint32_t hash = 5381;
        for (size_t i = 0; i < len; ++i) {
            wchar_t ch = str[i];

            if (ch >= L'A' && ch <= L'Z') ch += 32;
            hash = ((hash << 5) + hash) + static_cast<uint8_t>(ch & 0xFF);
        }
        return hash;
    }




    static HMODULE find_module_by_hash(uint32_t module_hash) {
#ifdef _WIN64
        auto* peb = reinterpret_cast<PEB_MINIMAL*>(__readgsqword(0x60));
#else
        auto* peb = reinterpret_cast<PEB_MINIMAL*>(__readfsdword(0x30));
#endif
        if (!peb || !peb->Ldr) return nullptr;

        auto* head = &peb->Ldr->InLoadOrderModuleList;
        auto* curr = head->Flink;

        int safety = 0;
        while (curr != head && curr != nullptr && ++safety < 256) {
            auto* entry = CONTAINING_RECORD(curr, LDR_DATA_TABLE_ENTRY_PEB, InLoadOrderLinks);

            if (entry->BaseDllName.Buffer && entry->BaseDllName.Length > 0) {
                uint32_t hash = djb2_hash_wide_lower(
                    entry->BaseDllName.Buffer,
                    entry->BaseDllName.Length / sizeof(wchar_t)
                );
                if (hash == module_hash) {
                    return static_cast<HMODULE>(entry->DllBase);
                }
            }
            curr = curr->Flink;
        }
        return nullptr;
    }




    static void* find_export_by_hash(HMODULE hModule, uint32_t func_hash) {
        if (!hModule) return nullptr;

        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hModule);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;

        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(
            reinterpret_cast<uint8_t*>(hModule) + dos->e_lfanew
        );
        if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;

        auto& exportDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (exportDir.VirtualAddress == 0 || exportDir.Size == 0) return nullptr;

        auto* exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(
            reinterpret_cast<uint8_t*>(hModule) + exportDir.VirtualAddress
        );

        auto* names    = reinterpret_cast<DWORD*>(reinterpret_cast<uint8_t*>(hModule) + exports->AddressOfNames);
        auto* ordinals = reinterpret_cast<WORD*>(reinterpret_cast<uint8_t*>(hModule) + exports->AddressOfNameOrdinals);
        auto* funcs    = reinterpret_cast<DWORD*>(reinterpret_cast<uint8_t*>(hModule) + exports->AddressOfFunctions);

        for (DWORD i = 0; i < exports->NumberOfNames; ++i) {
            const char* funcName = reinterpret_cast<const char*>(
                reinterpret_cast<uint8_t*>(hModule) + names[i]
            );
            if (djb2_hash_runtime(funcName) == func_hash) {
                WORD ordinal = ordinals[i];
                return reinterpret_cast<void*>(
                    reinterpret_cast<uint8_t*>(hModule) + funcs[ordinal]
                );
            }
        }
        return nullptr;
    }




    void* resolve_export_by_hash(uint32_t module_hash, uint32_t func_hash) {
        HMODULE hMod = find_module_by_hash(module_hash);
        if (!hMod) return nullptr;
        return find_export_by_hash(hMod, func_hash);
    }




    bool ResolvedAPIs::resolve_all() {

        pLoadLibraryW = reinterpret_cast<fn_LoadLibraryW>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::LoadLibraryW_));

        pGetProcAddress = reinterpret_cast<fn_GetProcAddress>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::GetProcAddress_));

        pGetModuleHandleW = reinterpret_cast<fn_GetModuleHandleW>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::GetModuleHandleW_));

        pCreateFileW = reinterpret_cast<fn_CreateFileW>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::CreateFileW_));

        pDeviceIoControl = reinterpret_cast<fn_DeviceIoControl>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::DeviceIoControl_));

        pCloseHandle = reinterpret_cast<fn_CloseHandle>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::CloseHandle_));

        pFindResourceW = reinterpret_cast<fn_FindResourceW>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::FindResourceW_));

        pLoadResource = reinterpret_cast<fn_LoadResource>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::LoadResource_));

        pLockResource = reinterpret_cast<fn_LockResource>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::LockResource_));

        pSizeofResource = reinterpret_cast<fn_SizeofResource>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::SizeofResource_));

        pVirtualAlloc = reinterpret_cast<fn_VirtualAlloc>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::VirtualAlloc_));

        pVirtualFree = reinterpret_cast<fn_VirtualFree>(
            resolve_export_by_hash(hashes::KERNEL32, hashes::VirtualFree_));


        bool ok = pLoadLibraryW && pGetProcAddress && pGetModuleHandleW
               && pCreateFileW && pDeviceIoControl && pCloseHandle;

        if (ok) {
            LOG_INFO("API Redirect: All critical APIs resolved via PEB walk");
        } else {
            LOG_ERROR("API Redirect: Failed to resolve one or more critical APIs");
        }
        return ok;
    }


    ResolvedAPIs& get_apis() {
        static ResolvedAPIs instance;
        static bool initialized = false;
        if (!initialized) {
            instance.resolve_all();
            initialized = true;
        }
        return instance;
    }

}
