#pragma once

#include <cstdint>
#include <windows.h>

namespace maxi::driver::api {





    constexpr uint32_t djb2_hash(const char* str) {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + static_cast<uint8_t>(*str);
            ++str;
        }
        return hash;
    }


    uint32_t djb2_hash_runtime(const char* str);




    namespace hashes {

        constexpr uint32_t KERNEL32    = djb2_hash("kernel32.dll");
        constexpr uint32_t NTDLL       = djb2_hash("ntdll.dll");
        constexpr uint32_t WIN32U      = djb2_hash("win32u.dll");
        constexpr uint32_t GDI32       = djb2_hash("gdi32.dll");


        constexpr uint32_t LoadLibraryW_       = djb2_hash("LoadLibraryW");
        constexpr uint32_t GetProcAddress_     = djb2_hash("GetProcAddress");
        constexpr uint32_t GetModuleHandleW_   = djb2_hash("GetModuleHandleW");
        constexpr uint32_t CreateFileW_        = djb2_hash("CreateFileW");
        constexpr uint32_t DeviceIoControl_    = djb2_hash("DeviceIoControl");
        constexpr uint32_t CloseHandle_        = djb2_hash("CloseHandle");
        constexpr uint32_t FindResourceW_      = djb2_hash("FindResourceW");
        constexpr uint32_t LoadResource_       = djb2_hash("LoadResource");
        constexpr uint32_t LockResource_       = djb2_hash("LockResource");
        constexpr uint32_t SizeofResource_     = djb2_hash("SizeofResource");
        constexpr uint32_t VirtualAlloc_       = djb2_hash("VirtualAlloc");
        constexpr uint32_t VirtualFree_        = djb2_hash("VirtualFree");
        constexpr uint32_t NtGdiDdDDISubmitCommand_ = djb2_hash("NtGdiDdDDISubmitCommand");
    }




    using fn_LoadLibraryW      = HMODULE(WINAPI*)(LPCWSTR);
    using fn_GetProcAddress    = FARPROC(WINAPI*)(HMODULE, LPCSTR);
    using fn_GetModuleHandleW  = HMODULE(WINAPI*)(LPCWSTR);
    using fn_CreateFileW       = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    using fn_DeviceIoControl   = BOOL(WINAPI*)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
    using fn_CloseHandle       = BOOL(WINAPI*)(HANDLE);
    using fn_FindResourceW     = HRSRC(WINAPI*)(HMODULE, LPCWSTR, LPCWSTR);
    using fn_LoadResource      = HGLOBAL(WINAPI*)(HMODULE, HRSRC);
    using fn_LockResource      = LPVOID(WINAPI*)(HGLOBAL);
    using fn_SizeofResource    = DWORD(WINAPI*)(HMODULE, HRSRC);
    using fn_VirtualAlloc      = LPVOID(WINAPI*)(LPVOID, SIZE_T, DWORD, DWORD);
    using fn_VirtualFree       = BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD);




    struct ResolvedAPIs {
        fn_LoadLibraryW      pLoadLibraryW      = nullptr;
        fn_GetProcAddress    pGetProcAddress     = nullptr;
        fn_GetModuleHandleW  pGetModuleHandleW   = nullptr;
        fn_CreateFileW       pCreateFileW        = nullptr;
        fn_DeviceIoControl   pDeviceIoControl    = nullptr;
        fn_CloseHandle       pCloseHandle        = nullptr;
        fn_FindResourceW     pFindResourceW      = nullptr;
        fn_LoadResource      pLoadResource       = nullptr;
        fn_LockResource      pLockResource       = nullptr;
        fn_SizeofResource    pSizeofResource     = nullptr;
        fn_VirtualAlloc      pVirtualAlloc       = nullptr;
        fn_VirtualFree       pVirtualFree        = nullptr;

        bool resolve_all();
    };


    ResolvedAPIs& get_apis();





    void* resolve_export_by_hash(uint32_t module_hash, uint32_t func_hash);

}
