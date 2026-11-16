#include "../../include/driver/driver_loader.hpp"
#include "../../include/driver/xor_crypt.hpp"
#include "../../include/driver/api_redirect.hpp"
#include "../../include/utils/logger.hpp"
#include "../../include/render/resource.h"

#include <windows.h>
#include <winsvc.h>
#include <shlobj.h>
#include <random>
#include <chrono>
#include <filesystem>
#include <format>

#pragma comment(lib, "advapi32.lib")

namespace maxi::driver::loader {





    std::wstring generate_random_name(size_t length) {

        auto seed = static_cast<uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        std::mt19937_64 rng(seed ^ 0xDEADBEEF13371337ULL);


        constexpr wchar_t charset[] =
            L"abcdefghijklmnopqrstuvwxyz"
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            L"0123456789";
        constexpr size_t charset_size = (sizeof(charset) / sizeof(wchar_t)) - 1;

        std::wstring result;
        result.reserve(length);


        std::uniform_int_distribution<size_t> letter_dist(0, 51);
        result += charset[letter_dist(rng)];

        std::uniform_int_distribution<size_t> char_dist(0, charset_size - 1);
        for (size_t i = 1; i < length; ++i) {
            result += charset[char_dist(rng)];
        }
        return result;
    }






    std::wstring get_hidden_drop_path() {
        wchar_t common_app_data[MAX_PATH] = {};


        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, common_app_data))) {

            wcscpy_s(common_app_data, L"C:\\ProgramData");
        }



        std::wstring hidden_dir = std::wstring(common_app_data)
            + L"\\Microsoft\\Crypto\\RSA\\MachineKeys";


        CreateDirectoryW(hidden_dir.c_str(), nullptr);

        return hidden_dir;
    }





    static bool write_driver_to_disk(const std::wstring& path,
                                     const std::vector<uint8_t>& data) {
        HANDLE hFile = CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_WRITE_THROUGH,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            LOG_ERROR("Driver Loader: Failed to create temp file: " +
                      std::to_string(GetLastError()));
            return false;
        }

        DWORD written = 0;
        BOOL ok = WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()),
                            &written, nullptr);
        CloseHandle(hFile);

        if (!ok || written != data.size()) {
            LOG_ERROR("Driver Loader: Write failed (wrote " +
                      std::to_string(written) + " of " +
                      std::to_string(data.size()) + " bytes)");

            DeleteFileW(path.c_str());
            return false;
        }

        return true;
    }





    static void secure_delete(const std::wstring& path) {

        HANDLE hFile = CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            0, nullptr,
            OPEN_EXISTING,
            FILE_FLAG_WRITE_THROUGH,
            nullptr
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER size{};
            GetFileSizeEx(hFile, &size);

            if (size.QuadPart > 0 && size.QuadPart < 64 * 1024 * 1024) {

                std::vector<uint8_t> zeros(static_cast<size_t>(size.QuadPart), 0);
                DWORD written = 0;
                SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
                WriteFile(hFile, zeros.data(), static_cast<DWORD>(zeros.size()),
                          &written, nullptr);
                FlushFileBuffers(hFile);
            }
            CloseHandle(hFile);
        }

        DeleteFileW(path.c_str());
    }




    static bool install_and_start_driver(const std::wstring& service_name,
                                         const std::wstring& driver_path) {
        SC_HANDLE hScm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (!hScm) {
            LOG_ERROR("Driver Loader: OpenSCManager failed: " +
                      std::to_string(GetLastError()));
            return false;
        }


        SC_HANDLE hExisting = OpenServiceW(hScm, service_name.c_str(), DELETE);
        if (hExisting) {
            DeleteService(hExisting);
            CloseServiceHandle(hExisting);
        }


        SC_HANDLE hService = CreateServiceW(
            hScm,
            service_name.c_str(),
            service_name.c_str(),
            SERVICE_ALL_ACCESS,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_IGNORE,
            driver_path.c_str(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );

        if (!hService) {
            DWORD err = GetLastError();
            CloseServiceHandle(hScm);
            LOG_ERROR("Driver Loader: CreateService failed: " + std::to_string(err));
            return false;
        }


        BOOL started = StartServiceW(hService, 0, nullptr);
        DWORD startErr = GetLastError();

        CloseServiceHandle(hService);
        CloseServiceHandle(hScm);

        if (!started && startErr != ERROR_SERVICE_ALREADY_RUNNING && startErr != ERROR_ALREADY_EXISTS) {
            LOG_ERROR("Driver Loader: StartService failed: " + std::to_string(startErr));
            return false;
        }

        MoveFileExW(driver_path.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);

        LOG_INFO("Driver Loader: Service '" +
                 std::string(service_name.begin(), service_name.end()) +
                 "' started successfully");
        return true;
    }





    static void clean_scm_entry(const std::wstring& service_name) {
        SC_HANDLE hScm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (!hScm) return;

        SC_HANDLE hService = OpenServiceW(hScm, service_name.c_str(),
                                          DELETE);
        if (hService) {

            DeleteService(hService);
            CloseServiceHandle(hService);

            LOG_INFO("Driver Loader: SCM entry cleaned for service: " +
                     std::string(service_name.begin(), service_name.end()));
        }

        CloseServiceHandle(hScm);


        std::wstring reg_path = L"SYSTEM\\CurrentControlSet\\Services\\" + service_name;
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, reg_path.c_str());


        std::wstring enum_path = L"SYSTEM\\CurrentControlSet\\Enum\\Root\\LEGACY_" + service_name;
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, enum_path.c_str());
    }




    DriverLoadResult auto_load_driver() {
        DriverLoadResult result;

        LOG_INFO("Driver Loader: Starting primary driver install...");

        HMODULE hSelf = GetModuleHandleW(nullptr);
        if (!hSelf) {
            result.error = "Failed to get module handle";
            LOG_ERROR("Driver Loader: " + result.error);
            return result;
        }


        std::vector<uint8_t> driver_bytes;
        HRSRC hRes = FindResourceW(hSelf, MAKEINTRESOURCE(IDR_DRIVER_IMAGE), RT_RCDATA);
        if (hRes) {
            HGLOBAL hData = LoadResource(hSelf, hRes);
            if (hData) {
                DWORD resSize = SizeofResource(hSelf, hRes);
                const uint8_t* pResData = static_cast<const uint8_t*>(LockResource(hData));
                if (pResData && resSize > 0) {
                    driver_bytes.assign(pResData, pResData + resSize);
                }
            }
        }

        if (driver_bytes.empty()) {
            result.error = "Failed to extract primary driver from resources";
            LOG_ERROR("Driver Loader: " + result.error);
            return result;
        }

        if (driver_bytes.size() < 2 || driver_bytes[0] != 'M' || driver_bytes[1] != 'Z') {
            result.error = "Decrypted resource is not a valid PE";
            LOG_ERROR("Driver Loader: " + result.error);
            return result;
        }

        LOG_INFO("Driver Loader: Decrypted " + std::to_string(driver_bytes.size()) + " bytes");




        std::wstring service_name = generate_random_name(10);
        std::wstring file_name    = generate_random_name(14) + L".sys";


        std::wstring drop_dir  = get_hidden_drop_path();
        std::wstring full_path = drop_dir + L"\\" + file_name;

        LOG_INFO("Driver Loader: Dropping to hidden path...");


        if (!write_driver_to_disk(full_path, driver_bytes)) {
            result.error = "Failed to write driver to disk";
            return result;
        }


        SecureZeroMemory(driver_bytes.data(), driver_bytes.size());
        driver_bytes.clear();


        if (!install_and_start_driver(service_name, full_path)) {

            secure_delete(full_path);
            result.error = "Failed to install/start driver via SCM";
            return result;
        }


        Sleep(500);

        result.success      = true;
        result.service_name = service_name;
        result.temp_path    = full_path;

        LOG_INFO("Driver Loader: Driver loaded successfully and active in kernel");
        return result;
    }




    bool auto_unload_driver(const std::wstring& service_name, const std::wstring& temp_path) {
        if (service_name.empty()) return false;

        clean_scm_entry(service_name);

        if (!temp_path.empty()) {
            if (DeleteFileW(temp_path.c_str()))
                LOG_INFO("Driver Loader: temp driver deleted");
        }

        LOG_INFO("Driver Loader: Unloaded service: " +
                 std::string(service_name.begin(), service_name.end()));
        return true;
    }




    DriverLoadResult auto_load_aux_driver() {
        DriverLoadResult result;

        LOG_INFO("Driver Loader: Starting aux backend install...");

        HMODULE hSelf = GetModuleHandleW(nullptr);
        if (!hSelf) {
            result.error = "Failed to get module handle";
            LOG_ERROR("Driver Loader: " + result.error);
            return result;
        }

        std::vector<uint8_t> driver_bytes;
        HRSRC hRes = FindResourceW(hSelf, MAKEINTRESOURCE(IDR_DRIVER_IMAGE_AUX), RT_RCDATA);
        if (hRes) {
            HGLOBAL hData = LoadResource(hSelf, hRes);
            if (hData) {
                DWORD resSize = SizeofResource(hSelf, hRes);
                const uint8_t* pResData = static_cast<const uint8_t*>(LockResource(hData));
                if (pResData && resSize > 0) {
                    driver_bytes.assign(pResData, pResData + resSize);
                }
            }
        }

        if (driver_bytes.empty()) {
            result.error = "Failed to extract aux driver from resources";
            LOG_ERROR("Driver Loader: " + result.error);
            return result;
        }

        if (driver_bytes.size() < 2 || driver_bytes[0] != 'M' || driver_bytes[1] != 'Z') {
            result.error = "Aux driver resource is not a valid PE";
            LOG_ERROR("Driver Loader: " + result.error);
            return result;
        }

        LOG_INFO("Driver Loader: Decrypted aux driver " + std::to_string(driver_bytes.size()) + " bytes");

        std::wstring service_name = generate_random_name(10);
        std::wstring file_name    = generate_random_name(14) + L".sys";

        std::wstring drop_dir  = get_hidden_drop_path();
        std::wstring full_path = drop_dir + L"\\" + file_name;

        LOG_INFO("Driver Loader: Dropping aux driver to hidden path...");

        if (!write_driver_to_disk(full_path, driver_bytes)) {
            result.error = "Failed to write aux driver to disk";
            return result;
        }

        SecureZeroMemory(driver_bytes.data(), driver_bytes.size());
        driver_bytes.clear();

        if (!install_and_start_driver(service_name, full_path)) {
            secure_delete(full_path);
            result.error = "Failed to install/start aux driver via SCM";
            return result;
        }

        Sleep(500);

        result.success      = true;
        result.service_name = service_name;
        result.temp_path    = full_path;

        LOG_INFO("Driver Loader: aux driver loaded successfully and active in kernel");
        return result;
    }

}
