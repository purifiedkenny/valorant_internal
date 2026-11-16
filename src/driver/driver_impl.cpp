#include "../../include/driver/idriver.hpp"
#include "../../include/driver/xor_crypt.hpp"
#include "../../include/driver/api_redirect.hpp"
#include "../../include/driver/driver_loader.hpp"
#include "../../include/utils/logger.hpp"
#include "../../include/driver/driver_data.h"
#include "../../include/render/resource.h"

#define BACKEND_SCAN_LIMIT 0x800000000ull
#include <cstdio>
#include "../../include/driver/backend.h"
#include <ntstatus.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <string.h>
#include <windows.h>
#include <winioctl.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>
#include <sstream>
#include <format>
#include <string>


namespace maxi::driver {




    static std::wstring backend_sys_name() {

        static const wchar_t enc[] = {
            0x4031,0x4005,0x4053,0x4027,0x400A,0x4014,0x401A,0x4004,
            0x4000,0x4007,0x4023,0x4018,0x4012,0x4015,0x4021,0x4020,
            0x4049
        };
        std::wstring s;
        s.reserve(17);
        for (int i = 0; i < 17; ++i) s += static_cast<wchar_t>(enc[i] ^ 0x33);
        return s;
    }

    static std::vector<uint8_t> load_driver_image() {

        HMODULE hSelf = nullptr;

        auto& apis = api::get_apis();
        if (apis.pGetModuleHandleW) {
            hSelf = apis.pGetModuleHandleW(nullptr);
        } else {
            hSelf = GetModuleHandleW(nullptr);
        }

        if (hSelf) {
            auto image = crypt::load_and_decrypt_resource(hSelf, IDR_DRIVER_IMAGE);
            if (!image.empty()) {
                LOG_INFO("Driver loaded from embedded resource (no disk footprint)");
                return image;
            }
            LOG_WARNING("Embedded resource not found, falling back to file system");
        }


        std::wstring driverPath = backend_sys_name();
        auto image = crypt::load_and_decrypt_file(driverPath);
        if (!image.empty()) {
            LOG_INFO("Driver loaded from encrypted file");
            return image;
        }

        LOG_ERROR("Failed to load driver image from any source");
        return {};
    }




    class DriverImpl : public IDriver {
    public:
        DriverImpl() : m_process_id(0), m_base_address(0), m_dtb(0) {}

        ~DriverImpl() override {
            unload();
        }


        bool setup() override {
            LOG_INFO("Setting up driver with AV evasion layers...");

            m_load_result = loader::auto_load_driver();
            if (!m_load_result.success) {
                LOG_ERROR("Auto-loader failed: " + m_load_result.error);
                return false;
            }
            LOG_INFO("SETUP SUB-STEP A: auto_load_driver complete");


            m_aux_load_result = loader::auto_load_aux_driver();
            if (!m_aux_load_result.success) {
                LOG_WARNING("Aux backend auto-loader failed: " + m_aux_load_result.error);
            } else {
                LOG_INFO("SETUP SUB-STEP A2: aux backend loaded");
            }


            auto& apis = api::get_apis();
            LOG_INFO("SETUP SUB-STEP B: API resolution done");
            if (!apis.pGetModuleHandleW || !apis.pGetProcAddress) {
                LOG_WARNING("Some APIs failed PEB resolution, using direct calls as fallback");
            }




            if (!m_vm.ping()) {
                LOG_ERROR("Driver device open/query failed - driver not loaded?");
                return false;
            }
            LOG_INFO(std::format("SETUP SUB-STEP E: Certified Kernel Driver Connected -> query OK (phys_total=0x{:X})", m_vm.m_phys_total));
            LOG_INFO("Driver successfully initialized - Physical Memory IOCTL & Kernel communications ready");
            return true;
        }

        void unload() override {
            if (m_vm.hDevice != INVALID_HANDLE_VALUE) {
                CloseHandle(m_vm.hDevice);
                m_vm.hDevice = INVALID_HANDLE_VALUE;
            }
            m_vm.driver_ok = false;

            if (!m_aux_load_result.service_name.empty()) {
                loader::auto_unload_driver(m_aux_load_result.service_name, m_aux_load_result.temp_path);
                m_aux_load_result = {};
            }

            if (!m_load_result.service_name.empty()) {
                loader::auto_unload_driver(m_load_result.service_name, m_load_result.temp_path);
                m_load_result = {};
            }
        }

        bool is_valid() const override {
            return m_vm.driver_ok && m_vm.hDevice != INVALID_HANDLE_VALUE;
        }


        bool attach_process(const std::wstring& process_name) override {
            return attach_by_name(process_name);
        }

        bool attach_process(uint32_t process_id) override {
            std::wstring name = process_name_from_id(process_id);
            if (name.empty()) {
                LOG_ERROR("Failed to resolve process name for PID: " + std::to_string(process_id));
                return false;
            }
            return attach_by_name(name);
        }

        uint32_t get_process_id() const override { return m_process_id; }
        uintptr_t get_base_address() const override { return m_base_address; }
        uintptr_t get_dtb() const override { return m_dtb; }


        bool move_mouse(int32_t x, int32_t y, uint16_t button_flags) override {
            (void)x; (void)y; (void)button_flags;
            LOG_WARNING("[READ-ONLY] move_mouse blocked - mouse injection disabled in ESP read-only mode");
            return false;
        }


        bool read_memory(uintptr_t address, void* buffer, size_t size) override {
            if (!address || !buffer || size == 0) return false;

            if (m_vm.m_dtb == 0) return false;
            return m_vm.read_memory(address, buffer, size);
        }

        bool write_memory(void* dest, void* src, std::size_t size) override {
            (void)dest; (void)src; (void)size;
            LOG_WARNING("[READ-ONLY] write_memory blocked - memory writes disabled in ESP read-only mode");
            return false;
        }



        bool stream_mode(HWND hwnd, uint32_t flag) override {
            (void)hwnd; (void)flag;
            return true;
        }


        uintptr_t get_kernel_base(const std::string& module_name) override {



            return resolve_kernel_module_base(module_name);
        }


        void* find_pattern(uintptr_t base, const char* pattern, const char* mask) override {
            if (!is_valid() || !pattern || !mask) return nullptr;

            const size_t CHUNK_SIZE = 4096;
            const size_t MAX_SCAN_SIZE = 0x10000000;
            const size_t pattern_length = strlen(mask);

            if (pattern_length == 0) return nullptr;

            std::vector<uint8_t> chunk_data(CHUNK_SIZE);
            std::vector<uint8_t> pattern_bytes;
            std::vector<bool> pattern_mask;


            if (!parse_pattern(pattern, mask, pattern_bytes, pattern_mask)) {
                return nullptr;
            }

            for (size_t offset = 0; offset < MAX_SCAN_SIZE; offset += CHUNK_SIZE) {
                uintptr_t current_address = base + offset;


                if (!read_memory(current_address, chunk_data.data(), CHUNK_SIZE)) {
                    break;
                }


                for (size_t i = 0; i <= CHUNK_SIZE - pattern_length; i++) {
                    if (match_pattern(chunk_data.data() + i, pattern_bytes, pattern_mask)) {
                        return reinterpret_cast<void*>(current_address + i);
                    }
                }


                if (offset + CHUNK_SIZE < MAX_SCAN_SIZE && pattern_length > 1) {

                    std::vector<uint8_t> overlap_data(pattern_length - 1);
                    if (read_memory(current_address + CHUNK_SIZE, overlap_data.data(), pattern_length - 1)) {

                        for (size_t i = CHUNK_SIZE - pattern_length + 1; i < CHUNK_SIZE; i++) {
                            std::vector<uint8_t> border_data;
                            border_data.insert(border_data.end(),
                                             chunk_data.begin() + i, chunk_data.end());
                            border_data.insert(border_data.end(),
                                             overlap_data.begin(),
                                             overlap_data.begin() + (pattern_length - (CHUNK_SIZE - i)));

                            if (match_pattern(border_data.data(), pattern_bytes, pattern_mask)) {
                                return reinterpret_cast<void*>(current_address + i);
                            }
                        }
                    }
                }
            }

            return nullptr;
        }

        void set_dir_base(void* dir) override {
            m_dtb = reinterpret_cast<uintptr_t>(dir);
            m_vm.m_dtb = m_dtb;
        }

    private:
        uint32_t    m_process_id = 0;
        uintptr_t   m_base_address = 0;
        uintptr_t   m_dtb = 0;
        loader::DriverLoadResult m_load_result;
        loader::DriverLoadResult m_aux_load_result;


        vm_t m_vm{};


        bool attach_by_name(const std::wstring& process_name) {
            if (!m_vm.driver_ok) return false;

            m_process_id = get_process_id_by_name(process_name);
            if (!m_process_id) {
                LOG_ERROR("Failed to find process: " + std::string(process_name.begin(), process_name.end()));
                return false;
            }
            LOG_INFO("Found process PID: " + std::to_string(m_process_id));


            backend_select_offsets();

            m_vm.m_pid = m_process_id;
            m_vm.m_peb = resolve_peb(m_process_id);

            std::string image(process_name.begin(), process_name.end());

            uint64_t dtb = m_vm.find_cr3(m_process_id, image.c_str());
            if (!dtb) {
                LOG_ERROR("CRITICAL SECURITY ERROR: Kernel driver failed to resolve CR3 (find_cr3)! Aborting to prevent usermode detection.");
                const auto& s = m_vm.last_fcr3_stats;
                std::ostringstream oss;
                oss << "[FCR3] scan stats: match=" << s.match << " pidok=" << s.pidok
                    << " dtbok=" << s.dtbok << " mzok=" << s.mzok
                    << " dtb_off=0x" << std::hex << s.dtb_off
                    << " readfail=" << std::dec << s.readfail
                    << " | peb=" << (m_vm.m_peb ? "ok" : "NONE (OpenProcess may be blocked)")
                    << " | phys_total=0x" << std::hex << m_vm.m_phys_total
                    << " phys_max=0x" << m_vm.m_phys_max;
                LOG_INFO(oss.str());
                std::ostringstream rs;
                rs << "[FCR3] coverage: ranges=" << s.ranges
                   << " scanned=0x" << std::hex << s.scanned
                   << " lo=0x" << s.range_lo << " hi=0x" << s.range_hi;
                LOG_INFO(rs.str());
                if (s.ep_dumped) {
                    std::ostringstream hd;
                    hd << "[FCR3] EPROCESS hdr: ";
                    for (int i = 0; i < 0x40; i += 8) {
                        uint64_t v;
                        memcpy(&v, s.ep_hdr + i, 8);
                        hd << std::hex << "+0x" << i << "=0x" << v << " ";
                    }
                    LOG_INFO(hd.str());
                } else {
                    LOG_INFO("[FCR3] EPROCESS hdr dump: UNAVAILABLE (read_phys fail)");
                }
                return false;
            }
            m_vm.m_dtb = dtb;

            if (m_vm.m_peb) {
                m_vm.m_base_address = m_vm.read<uintptr_t>(m_vm.m_peb + 0x10);
            }

            m_base_address = m_vm.m_base_address;
            m_dtb = m_vm.m_dtb;

            if (!m_base_address) {
                LOG_ERROR("CRITICAL SECURITY ERROR: Kernel driver failed to get base address! Aborting.");
                return false;
            }
            LOG_INFO("Kernel Base address resolved: 0x" + [](uintptr_t v) { std::ostringstream o; o << std::hex << v; return o.str(); }(m_base_address));
            LOG_INFO("Kernel DTB resolved: 0x" + [](uintptr_t v) { std::ostringstream o; o << std::hex << v; return o.str(); }(m_dtb));
            return true;
        }

        std::wstring process_name_from_id(uint32_t pid) {
            std::wstring result;
            auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return result;
            PROCESSENTRY32W pe{};
            pe.dwSize = sizeof(pe);
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (pe.th32ProcessID == pid) { result = pe.szExeFile; break; }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return result;
        }

        uint32_t get_process_id_by_name(const std::wstring& process_name) {
            auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE) return 0;

            PROCESSENTRY32W process_entry{};
            process_entry.dwSize = sizeof(process_entry);

            if (!Process32FirstW(snapshot, &process_entry)) {
                CloseHandle(snapshot);
                return 0;
            }

            do {
                if (process_name == process_entry.szExeFile) {
                    CloseHandle(snapshot);
                    return process_entry.th32ProcessID;
                }
            } while (Process32NextW(snapshot, &process_entry));

            CloseHandle(snapshot);
            return 0;
        }

        uint64_t resolve_peb(uint32_t pid) {
            HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
            if (!h) return 0;

            typedef LONG(NTAPI* pfnNtQIP)(HANDLE, ULONG, PVOID, ULONG, PULONG);
            static pfnNtQIP fn = nullptr;
            if (!fn) {
                HMODULE hNt = GetModuleHandleA("ntdll.dll");
                if (hNt) fn = (pfnNtQIP)GetProcAddress(hNt, "NtQueryInformationProcess");
            }

            struct PBI { PVOID Reserved1; PVOID PebBaseAddress; PVOID Reserved2[2]; ULONG_PTR UniqueProcessId; PVOID Reserved3; } pbi{};
            uint64_t peb = 0;
            if (fn && fn(h, 0 /*ProcessBasicInformation*/, &pbi, sizeof(pbi), nullptr) == 0) {
                peb = (uint64_t)pbi.PebBaseAddress;
            }
            CloseHandle(h);
            return peb;
        }

        uintptr_t resolve_kernel_module_base(const std::string& module_name) {
            typedef LONG(NTAPI* pfnNtQSI)(ULONG, PVOID, ULONG, PULONG);
            static pfnNtQSI fn = nullptr;
            if (!fn) {
                HMODULE hNt = GetModuleHandleA("ntdll.dll");
                if (hNt) fn = (pfnNtQSI)GetProcAddress(hNt, "NtQuerySystemInformation");
            }
            if (!fn) return 0;

            const ULONG SystemModuleInformation = 11;
            ULONG len = 0;
            fn(SystemModuleInformation, nullptr, 0, &len);
            if (!len) return 0;

            std::vector<uint8_t> buf(len + 1024);
            ULONG ret = 0;
            if (fn(SystemModuleInformation, buf.data(), (ULONG)buf.size(), &ret) != 0) return 0;

            struct RTL_PROCESS_MODULE_INFORMATION {
                HANDLE  Section;
                PVOID   MappedBase;
                PVOID   ImageBase;
                ULONG   ImageSize;
                ULONG   Flags;
                USHORT  LoadOrderIndex;
                USHORT  InitOrderIndex;
                USHORT  LoadCount;
                USHORT  OffsetToFileName;
                UCHAR   FullPathName[256];
            };
            struct RTL_PROCESS_MODULES {
                ULONG NumberOfModules;
                RTL_PROCESS_MODULE_INFORMATION Modules[1];
            };

            auto* mods = reinterpret_cast<RTL_PROCESS_MODULES*>(buf.data());
            for (ULONG i = 0; i < mods->NumberOfModules; ++i) {
                const auto& m = mods->Modules[i];
                const char* fname = reinterpret_cast<const char*>(m.FullPathName) + m.OffsetToFileName;
                if (_stricmp(fname, module_name.c_str()) == 0) {
                    return reinterpret_cast<uintptr_t>(m.ImageBase);
                }
            }
            return 0;
        }


        bool parse_pattern(const char* pattern, const char* mask,
                          std::vector<uint8_t>& pattern_bytes, std::vector<bool>& pattern_mask) {
            const size_t length = strlen(mask);
            pattern_bytes.reserve(length);
            pattern_mask.reserve(length);

            std::istringstream pattern_stream(pattern);
            std::string byte_str;
            size_t mask_index = 0;

            while (pattern_stream >> byte_str && mask_index < length) {
                if (mask[mask_index] == '?') {
                    pattern_bytes.push_back(0x00);
                    pattern_mask.push_back(false);
                } else {
                    try {
                        uint8_t byte_val = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
                        pattern_bytes.push_back(byte_val);
                        pattern_mask.push_back(true);
                    } catch (const std::exception&) {
                        return false;
                    }
                }
                mask_index++;
            }

            return mask_index == length;
        }


        bool match_pattern(const uint8_t* data, const std::vector<uint8_t>& pattern_bytes,
                         const std::vector<bool>& pattern_mask) {
            for (size_t i = 0; i < pattern_bytes.size(); i++) {
                if (pattern_mask[i] && data[i] != pattern_bytes[i]) {
                    return false;
                }
            }
            return true;
        }
    };


    std::shared_ptr<IDriver> create_driver() {
        return std::make_shared<DriverImpl>();
    }

}
