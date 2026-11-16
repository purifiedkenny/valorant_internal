#pragma once
#include <Windows.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>
#include <type_traits>
#include <algorithm>






















#define IOCTL_BACKEND_QUERY 0x0022218C
#define IOCTL_BACKEND_MAP   0x80102040
#define IOCTL_BACKEND_UNMAP 0x80102044
#define IOCTL_BACKEND_PORT_IN  0x80102050
#define IOCTL_BACKEND_PORT_OUT 0x80102054

#define BACKEND_PAGE_SIZE 0x1000ull

#define BACKEND_READ_WINDOW 0x00800000ull

static_assert(IOCTL_BACKEND_QUERY == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x863, METHOD_BUFFERED, FILE_ANY_ACCESS),
    "IOCTL_BACKEND_QUERY does not match the expected CTL_CODE");
static_assert(IOCTL_BACKEND_MAP == CTL_CODE(0x8010, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS),
    "IOCTL_BACKEND_MAP does not match the expected CTL_CODE");
static_assert(IOCTL_BACKEND_UNMAP == CTL_CODE(0x8010, 0x811, METHOD_BUFFERED, FILE_ANY_ACCESS),
    "IOCTL_BACKEND_UNMAP does not match the expected CTL_CODE");
static_assert(IOCTL_BACKEND_PORT_IN == CTL_CODE(0x8010, 0x814, METHOD_BUFFERED, FILE_ANY_ACCESS),
    "IOCTL_BACKEND_PORT_IN does not match the expected CTL_CODE");
static_assert(IOCTL_BACKEND_PORT_OUT == CTL_CODE(0x8010, 0x815, METHOD_BUFFERED, FILE_ANY_ACCESS),
    "IOCTL_BACKEND_PORT_OUT does not match the expected CTL_CODE");
static_assert(0x2E3F8C20 == CTL_CODE(0x2E3F, 0x2308, METHOD_BUFFERED, FILE_ANY_ACCESS),
    "AUX CR3 IOCTL does not match the expected CTL_CODE");
static_assert(BACKEND_READ_WINDOW == 0x800000ull, "BACKEND_READ_WINDOW must be 8 MB");


namespace backend_str {
    inline std::string dec(const uint8_t* enc, size_t n, uint8_t k) {
        std::string s(n, '\0');
        for (size_t i = 0; i < n; ++i) s[i] = static_cast<char>(enc[i] ^ k);
        return s;
    }

    inline const char* dev_backend() {
        static const uint8_t e[] = { 0x11,0x11,0x63,0x11,0x1D,0x0A,0x1F,0x05,0x22,0x3E,0x39,0x0E,0x22,0x23,0x39,0x3F,0x22,0x21 };
        static std::string s = dec(e, sizeof(e), 0x4D);
        return s.c_str();
    }

    inline const char* dev_aux() {
        static const uint8_t e[] = { 0x06,0x06,0x74,0x06,0x11,0x0A,0x08,0x16 };
        static std::string s = dec(e, sizeof(e), 0x5A);
        return s.c_str();
    }
}

inline unsigned long long g_backend_remaps = 0;
#ifndef BACKEND_SCAN_LIMIT
#define BACKEND_SCAN_LIMIT 0x1000000000ull
#endif

#ifndef BACKEND_OFFSET_DTB
#define BACKEND_OFFSET_DTB 0x28
#endif
#ifndef BACKEND_OFFSET_UNIQUEPID
#define BACKEND_OFFSET_UNIQUEPID 0x440
#endif
#ifndef BACKEND_OFFSET_IMAGEFILENAME
#define BACKEND_OFFSET_IMAGEFILENAME 0x5a8
#endif

struct backend_eprocess_entry {
    uint32_t min_build;
    uint32_t max_build;
    uint32_t pid_off;
    uint32_t img_off;
    const char* verified;
};
inline backend_eprocess_entry k_eprocess_table[] = {
    { 26100,  0xFFFFFFFFu, 0x1D0, 0x338, "VERIFIED (Win11 24H2-25H2, KernelArchive)" },
    { 22000,  26099,       0x440, 0x5A8, "VERIFIED (Win11 21H2-23H2, KernelArchive)" },
    { 19041,  19045,       0x440, 0x5A8, "VERIFIED (Win10 2004-22H2, KernelArchive + VM)" },
    { 18362,  18363,       0x2E8, 0x450, "VERIFIED (Win10 1903/1909, KernelArchive)" },
    { 17763,  18361,       0x2E0, 0x450, "VERIFIED (Win10 1809, KernelArchive)" },
    { 16299,  17762,       0x2E0, 0x450, "VERIFIED (Win10 1709-1803, KernelArchive)" },
};

inline uint64_t backend_pid_off = BACKEND_OFFSET_UNIQUEPID;
inline uint64_t backend_img_off = BACKEND_OFFSET_IMAGEFILENAME;

inline uint32_t backend_get_build_number() {
    typedef LONG(WINAPI* fn_t)(OSVERSIONINFOEXW*);
    static fn_t fn = (fn_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    OSVERSIONINFOEXW vi{ sizeof(OSVERSIONINFOEXW) };
    if (fn && fn(&vi) == 0) return vi.dwBuildNumber;
    return 0;
}

inline void backend_select_offsets() {
    uint32_t build = backend_get_build_number();
    if (!build) return;
    for (const auto& e : k_eprocess_table) {
        if (build >= e.min_build && build <= e.max_build) {
            backend_pid_off = e.pid_off;
            backend_img_off = e.img_off;
            printf("[maxi] EPROCESS offsets: build=%u -> pid_off=0x%llX img_off=0x%llX (%s)\n",
                build, (unsigned long long)backend_pid_off, (unsigned long long)backend_img_off, e.verified);
            return;
        }
    }
    printf("[maxi] EPROCESS offsets: build=%u unknown -> using default (VERIFIED): pid_off=0x%llX img_off=0x%llX\n",
        build, (unsigned long long)backend_pid_off, (unsigned long long)backend_img_off);
}


struct backend_query_req {
    uint32_t magic;
    uint32_t size;
    uint32_t flags;
};
struct backend_query_resp {
    uint32_t magic;
    uint32_t size;
    uint32_t status;
};


#pragma pack(push, 1)
struct backend_map_req {
    uint64_t size;
    uint64_t phys;
    uint64_t out_section;
    uint64_t out_mapped;
    uint64_t out_object;
};
#pragma pack(pop)

static_assert(sizeof(backend_query_req) == 12, "backend_query_req must be 12 bytes");
static_assert(sizeof(backend_query_resp) == 12, "backend_query_resp must be 12 bytes");
static_assert(sizeof(backend_map_req) == 40, "backend_map_req must be 40 bytes");
static_assert(offsetof(backend_map_req, size) == 0x00, "backend_map_req.size must be at +0x00");
static_assert(offsetof(backend_map_req, phys) == 0x08, "backend_map_req.phys must be at +0x08");
static_assert(offsetof(backend_map_req, out_section) == 0x10, "backend_map_req.out_section must be at +0x10");
static_assert(offsetof(backend_map_req, out_mapped) == 0x18, "backend_map_req.out_mapped must be at +0x18");
static_assert(offsetof(backend_map_req, out_object) == 0x20, "backend_map_req.out_object must be at +0x20");

struct batch_read_entry {
    uint64_t address = 0;
    size_t size = 0;
    uint32_t offset = 0;
};

typedef LONG NTSTATUS;
typedef struct _BACKEND_PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PVOID PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} BACKEND_PROCESS_BASIC_INFORMATION;

inline NTSTATUS backend_nt_query_information_process(HANDLE h, ULONG cls, PVOID buf, ULONG len, PULONG retlen) {
    typedef NTSTATUS(__stdcall* fn_t)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static fn_t fn = (fn_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
    if (!fn) return (NTSTATUS)0xC0000001ul;
    return fn(h, cls, buf, len, retlen);
}

struct backend_ram_range { uint64_t start; uint64_t end; };

struct backend_raw_smbios {
    BYTE  Used20CallingMethod;
    BYTE  SMBIOSMajorVersion;
    BYTE  SMBIOSMinorVersion;
    BYTE  DmiRevision;
    DWORD Length;
    BYTE  SMBIOSTableData[1];
};

inline std::vector<backend_ram_range> backend_get_ram_ranges() {
    std::vector<backend_ram_range> out;
    UINT sz = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
    if (!sz) return out;
    std::vector<BYTE> buf(sz);
    if (GetSystemFirmwareTable('RSMB', 0, buf.data(), sz) != sz) return out;
    const backend_raw_smbios* raw = reinterpret_cast<const backend_raw_smbios*>(buf.data());
    const BYTE* p = raw->SMBIOSTableData;
    const BYTE* endp = p + raw->Length;
    while (p + 4 <= endp) {
        const BYTE type = p[0];
        const BYTE len = p[1];
        if (len < 4 || p + len > endp) break;
        if (type == 0x00 && len == 0x04 && p[2] == 0 && p[3] == 0) break;
        if (type == 0x13 && len >= 0x0F) {
            const uint32_t s = *(const uint32_t*)(p + 4);
            const uint32_t e = *(const uint32_t*)(p + 8);
            if (e > s)
                out.push_back({ (uint64_t)s * 0x10000ull, (uint64_t)(e + 1) * 0x10000ull });
        } else if (type == 0x14 && len >= 0x13) {
            const uint64_t s = *(const uint64_t*)(p + 4);
            const uint64_t e = *(const uint64_t*)(p + 12);
            if (e > s)
                out.push_back({ s * 0x10000ull, (e + 1) * 0x10000ull });
        }
        p += len;
        while (p < endp && *p) p += (size_t)strlen((const char*)p) + 1;
        if (p < endp) p++;
    }
    return out;
}

inline bool backend_phys_in_ram(uint64_t phys, const std::vector<backend_ram_range>& ranges) {
    for (const auto& r : ranges)
        if (phys >= r.start && phys < r.end) return true;
    return false;
}

struct vm_t {
    uint64_t m_base_address = 0;
    uint64_t m_peb = 0;
    DWORD m_pid = 0;
    uint64_t m_dtb = 0;

    HANDLE hDevice = INVALID_HANDLE_VALUE;
    bool driver_ok = false;
    uint64_t m_phys_total = 0;

    uint64_t m_phys_max = 0;

    struct fcr3_stats_t {
        uint64_t match = 0, pidok = 0, dtbok = 0, mzok = 0;
        uint32_t dtb_off = 0xFFFFFFFF;
        uint32_t readfail = 0;
        bool     ep_dumped = false;
        uint8_t  ep_hdr[0x40] = {};
        uint64_t scanned = 0;
        uint32_t ranges = 0;
        uint64_t range_lo = 0, range_hi = 0;
    };
    fcr3_stats_t last_fcr3_stats;


    inline bool ping() {
        if (driver_ok && hDevice != INVALID_HANDLE_VALUE)
            return query();
        hDevice = CreateFileA(backend_str::dev_backend(),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hDevice == INVALID_HANDLE_VALUE) { driver_ok = false; return false; }
        driver_ok = query();
        if (driver_ok) {
            MEMORYSTATUSEX msx{ sizeof(MEMORYSTATUSEX) };
            if (GlobalMemoryStatusEx(&msx)) m_phys_total = msx.ullTotalPhys;
            if (!m_phys_max) {
                for (const auto& rg : backend_get_ram_ranges())
                    if (rg.end > m_phys_max) m_phys_max = rg.end;
                if (m_phys_max < m_phys_total)
                    m_phys_max = m_phys_total + 0x100000000ull;
            }
        }
        return driver_ok;
    }















    HANDLE hAux = INVALID_HANDLE_VALUE;
    bool aux_ok = false;

    static void log_ioctl(const char* name, BOOL ok, DWORD bytes, DWORD error) {
        if (!ok) {
            printf("[maxi] %s failed: GetLastError=%lu\n", name, error);
        } else {
            printf("[maxi] %s succeeded (ERROR_SUCCESS, bytes=%lu)\n", name, bytes);
        }
    }

    inline bool aux_open() {
        if (aux_ok && hAux != INVALID_HANDLE_VALUE) return true;
        hAux = CreateFileA(backend_str::dev_aux(),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        aux_ok = (hAux != INVALID_HANDLE_VALUE);
        return aux_ok;
    }


    inline uint64_t aux_get_cr3(DWORD pid) {
        if (!aux_open()) return 0;
        uint8_t buf[0x100] = {};
        *(uint32_t*)(buf + 0x00) = pid;
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(hAux, 0x2E3F8C20,
            buf, sizeof(buf), buf, sizeof(buf), &ret, nullptr);
        const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
        log_ioctl("AUX_CR3", ok, ret, error);
        if (!ok) return 0;
        uint64_t cr3 = *(uint64_t*)(buf + 0x40);
        if (!cr3 || (cr3 & 0xFFF) || cr3 >= m_phys_max) return 0;
        return cr3;
    }


    inline bool query() {
        if (hDevice == INVALID_HANDLE_VALUE) return false;
        backend_query_req req{ 1, sizeof(backend_query_req), 0 };
        backend_query_resp resp{};
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_BACKEND_QUERY,
            &req, sizeof(req), &resp, sizeof(resp), &ret, nullptr);
        const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
        log_ioctl("IOCTL_BACKEND_QUERY", ok, ret, error);
        if (!ok) return false;
        if (ret < sizeof(resp)) {
            printf("[maxi] IOCTL_BACKEND_QUERY protocol mismatch: ret=%lu expected=%zu\n",
                ret, sizeof(resp));
            return false;
        }
        if (resp.magic != 1 || resp.size != sizeof(resp) || resp.status != 1) {
            printf("[maxi] IOCTL_BACKEND_QUERY protocol mismatch: magic=%u size=%u status=%u\n",
                resp.magic, resp.size, resp.status);
            return false;
        }
        return true;
    }

    inline bool map_window(uint64_t phys, uint64_t size, uint64_t* mapped, uint64_t* section, uint64_t* object) {
        if (!driver_ok || hDevice == INVALID_HANDLE_VALUE) return false;
        backend_map_req req{};
        req.size = size;
        req.phys = phys;
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_BACKEND_MAP,
            &req, sizeof(req), &req, sizeof(req), &ret, nullptr);
        const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
        log_ioctl("IOCTL_BACKEND_MAP", ok, ret, error);
        if (!ok) return false;
        if (!req.out_mapped) {
            printf("[maxi] IOCTL_BACKEND_MAP protocol mismatch: section=0x%llX mapped=0x%llX object=0x%llX\n",
                (unsigned long long)req.out_section,
                (unsigned long long)req.out_mapped,
                (unsigned long long)req.out_object);
            return false;
        }
        if (mapped) *mapped = req.out_mapped;
        if (section) *section = req.out_section;
        if (object) *object = req.out_object;
        return true;
    }


    inline bool unmap(uint64_t section, uint64_t mapped, uint64_t object) {
        if (!driver_ok || hDevice == INVALID_HANDLE_VALUE) return false;
        backend_map_req req{};
        req.out_section = section;
        req.out_mapped = mapped;
        req.out_object = object;
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_BACKEND_UNMAP,
            &req, sizeof(req), &req, sizeof(req), &ret, nullptr);
        const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
        log_ioctl("IOCTL_BACKEND_UNMAP", ok, ret, error);
        return ok != FALSE;
    }

    inline bool safe_read(const void* src, void* dst, size_t n) {
        __try {
            memcpy(dst, src, n);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }



    struct backend_win { uint64_t phys = ~0ull, size = 0, va = 0, section = 0, object = 0, stamp = 0; };
    static constexpr int BACKEND_WIN_SLOTS = 32;
    static_assert(BACKEND_WIN_SLOTS == 32, "BACKEND_WIN_SLOTS must be 32");
    inline backend_win* wins() { static thread_local backend_win w[BACKEND_WIN_SLOTS]; return w; }
    inline uint64_t& win_clock() { static thread_local uint64_t c = 0; return c; }

    inline void drop_win(backend_win& w) {
        if (w.va) unmap(w.section, w.va, w.object);
        w.phys = ~0ull; w.size = 0; w.va = 0; w.section = 0; w.object = 0; w.stamp = 0;
    }

    inline void flush_window() {
        backend_win* w = wins();
        for (int i = 0; i < BACKEND_WIN_SLOTS; ++i) drop_win(w[i]);
    }

    inline backend_win* find_window(uint64_t phys, size_t size) {
        backend_win* w = wins();
        for (int i = 0; i < BACKEND_WIN_SLOTS; ++i) {
            backend_win& s = w[i];
            if (s.va && phys >= s.phys && phys + size <= s.phys + s.size)
                return &s;
        }
        return nullptr;
    }

    inline bool ensure_window(uint64_t phys, size_t size) {
        if (backend_win* hit = find_window(phys, size)) { hit->stamp = ++win_clock(); return true; }
        if (phys + size > m_phys_max) return false;
        backend_win* victim = nullptr;
        backend_win* w = wins();
        for (int i = 0; i < BACKEND_WIN_SLOTS; ++i)
            if (!w[i].va) { victim = &w[i]; break; }
        if (!victim) {
            victim = &w[0];
            for (int i = 1; i < BACKEND_WIN_SLOTS; ++i)
                if (w[i].stamp < victim->stamp) victim = &w[i];
            drop_win(*victim);
        }
        uint64_t wbase = phys & ~(BACKEND_READ_WINDOW - 1);
        uint64_t wsize = (std::min)((uint64_t)BACKEND_READ_WINDOW, m_phys_max > wbase ? m_phys_max - wbase : (uint64_t)BACKEND_READ_WINDOW);
        if (phys + size > wbase + wsize) return false;
        if (!map_window(wbase, wsize, &victim->va, &victim->section, &victim->object)) return false;
        g_backend_remaps++;
        victim->phys = wbase;
        victim->size = wsize;
        victim->stamp = ++win_clock();
        return true;
    }

    inline bool read_phys(uint64_t phys, void* out, size_t size) {
        if (!driver_ok || !out || !size) return false;
        if (!ensure_window(phys, size)) return false;
        backend_win* s = find_window(phys, size);
        if (!s) return false;
        if (!safe_read((const uint8_t*)s->va + (phys - s->phys), out, size)) {
            drop_win(*s);
            return false;
        }
        return true;
    }


    inline bool write_phys(uint64_t phys, const void* src, size_t size) {
        if (!driver_ok || !src || !size) return false;
        if (!ensure_window(phys, size)) return false;
        backend_win* s = find_window(phys, size);
        if (!s) return false;
        memcpy((uint8_t*)s->va + (phys - s->phys), src, size);
        return true;
    }

    template<typename T>
    inline bool write(uint64_t address, const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "write requires trivially copyable");
        uint64_t pa = 0;
        if (!virt_to_phys(address, &pa)) return false;
        return write_phys(pa, &value, sizeof(T));
    }


    inline bool attach(const char* name) {
        if (!driver_ok) return false;
        if (!name || !name[0]) return false;

        backend_select_offsets();

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        wchar_t wname[64] = {};
        MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 64);
        PROCESSENTRY32W pe{ sizeof(PROCESSENTRY32W) };
        bool found = false;
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, wname) == 0) { m_pid = pe.th32ProcessID; found = true; break; }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        if (!found) { m_pid = 0; return false; }

        m_peb = 0;
        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_pid);
        if (h) {
            BACKEND_PROCESS_BASIC_INFORMATION pbi{};
            if (backend_nt_query_information_process(h, 0 /*ProcessBasicInformation*/, &pbi, sizeof(pbi), nullptr) == 0)
                m_peb = (uint64_t)pbi.PebBaseAddress;
            CloseHandle(h);
        }

        m_dtb = find_cr3(m_pid);


        if (m_dtb && m_peb)
            m_base_address = read<uintptr_t>(m_peb + 0x10);

        return m_pid != 0;
    }
    inline bool attach(const std::string& s) { return attach(s.c_str()); }

    inline bool is_valid_cr3(uint64_t cr3) {
        if (!cr3 || (cr3 & 0xFFF)) return false;
        if (cr3 >= m_phys_max) return false;
        uint64_t pml4e = 0;
        if (!read_phys(cr3 + 0xFF8, &pml4e, 8)) return false;
        if (!(pml4e & 1)) return false;
        uint64_t pdpt = pml4e & 0x000FFFFFFFFFF000ull;
        if (pdpt >= m_phys_max) return false;
        uint64_t pdpte = 0;
        if (!read_phys(pdpt, &pdpte, 8)) return false;
        if (!(pdpte & 1)) return false;
        return true;
    }


    struct backend_cr3_cache_t {
        char exe[32];
        uint64_t create_time;
        uint64_t cr3;
        uint64_t ep_phys;
    };

    inline std::string backend_cache_path() {
        char tmp[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tmp);
        return std::string(tmp) + "backend_cr3_cache.dat";
    }

    inline uint64_t backend_process_create_time(DWORD pid) {


        typedef LONG(NTAPI* fn_qsi)(ULONG, PVOID, ULONG, PULONG);
        static fn_qsi qsi = (fn_qsi)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
        if (!qsi) return 0;
        ULONG size = 0;
        qsi(5, nullptr, 0, &size);
        if (!size) return 0;
        std::vector<uint8_t> buf(size + 0x100);
        if (qsi(5, buf.data(), (ULONG)buf.size(), &size) != 0) return 0;
        const uint8_t* p = buf.data();
        while (true) {
            const uint32_t next = *(const uint32_t*)p;
            const uint64_t ctime = *(const uint64_t*)(p + 0x20);

            const uint64_t p1 = *(const uint64_t*)(p + 0x28);
            const uint64_t p2 = *(const uint64_t*)(p + 0x50);
            if ((uint32_t)p1 == pid || (uint32_t)p2 == pid) return ctime;
            if (!next) break;
            p += next;
        }
        return 0;
    }

    inline bool backend_cr3_cache_load(const char* exe, DWORD pid, uint64_t* cr3) {
        if (!exe || !cr3) return false;
        HANDLE h = CreateFileA(backend_cache_path().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        backend_cr3_cache_t c{};
        DWORD rd = 0;
        bool ok = ReadFile(h, &c, sizeof(c), &rd, nullptr) && rd == sizeof(c) && c.cr3 && c.create_time &&
                  _strnicmp(c.exe, exe, 31) == 0 &&
                  c.create_time == backend_process_create_time(pid);
        CloseHandle(h);
        if (ok) *cr3 = c.cr3;
        return ok;
    }

    inline void backend_cr3_cache_save(const char* exe, DWORD pid, uint64_t cr3, uint64_t ep_phys) {
        if (!exe || !cr3) return;
        backend_cr3_cache_t c{};
        strncpy(c.exe, exe, 31);
        c.exe[31] = 0;
        c.create_time = backend_process_create_time(pid);
        c.cr3 = cr3;
        c.ep_phys = ep_phys;
        if (!c.create_time) return;
        HANDLE h = CreateFileA(backend_cache_path().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;
        DWORD wr = 0;
        WriteFile(h, &c, sizeof(c), &wr, nullptr);
        CloseHandle(h);
    }


    inline uint64_t find_cr3(DWORD pid, const char* image = "cod.exe", uint64_t verify_base = 0) {
        if (!driver_ok || !m_phys_total) return 0;
        if (!image || !image[0]) return 0;
        if (!m_phys_max) m_phys_max = m_phys_total + 0x100000000ull;


        uint64_t aux_cr3 = aux_get_cr3(pid);
        if (aux_cr3) {
            printf("[maxi] aux CR3 hit: 0x%llX\n", (unsigned long long)aux_cr3);
            if (m_peb) {
                const uint64_t prev = m_dtb;
                m_dtb = aux_cr3;
                uint64_t base = read<uint64_t>(m_peb + 0x10);
                uint64_t pa = 0; uint16_t mz = 0;
                bool ok = base && base >= 0x10000 && virt_to_phys(base, &pa) && read_phys(pa, &mz, 2) && mz == 0x5A4D;
                m_dtb = prev;
                if (ok) {
                    backend_cr3_cache_save(image, pid, aux_cr3, 0);
                    return aux_cr3;
                }
                printf("[maxi] aux CR3 MZ validation failed -> falling back to scan\n");
            } else {
                backend_cr3_cache_save(image, pid, aux_cr3, 0);
                return aux_cr3;
            }
        } else {
            printf("[maxi] no aux CR3 (driver not loaded or PID protected) -> physical scan\n");
        }

        uint64_t cached = 0;
        if (backend_cr3_cache_load(image, pid, &cached)) {
            printf("[maxi] cache load OK cr3=0x%llX is_valid=%d\n", (unsigned long long)cached, (int)is_valid_cr3(cached));
            const uint64_t prev = m_dtb;
            m_dtb = cached;
            bool ok = cached && !(cached & 0xFFF) && cached < m_phys_max;
            if (ok && m_peb) {
                uint64_t base = read<uint64_t>(m_peb + 0x10);
                uint64_t pa = 0; uint16_t mz = 0;
                ok = base && base >= 0x10000 && virt_to_phys(base, &pa) && read_phys(pa, &mz, 2) && mz == 0x5A4D;
            }
            m_dtb = prev;
            if (ok) {
                printf("[maxi] CR3 cache hit: 0x%llX (scan skipped)\n", (unsigned long long)cached);
                return cached;
            }
        }

        backend_select_offsets();
        last_fcr3_stats = {};

        static std::vector<backend_ram_range> s_ranges = backend_get_ram_ranges();
        if (s_ranges.empty()) {
            s_ranges.push_back({ 0x100000ull, 0xC0000000ull });
            const uint64_t hi = (std::min)(m_phys_max, (uint64_t)BACKEND_SCAN_LIMIT);
            if (hi > 0x100000000ull)
                s_ranges.push_back({ 0x100000000ull, hi });
            printf("[maxi] MCE guard: SMBIOS memory map unavailable, scanning 1MB-3GB + 4GB-%llX\n", (unsigned long long)hi);
        }

        size_t nlen = strlen(image);
        if (nlen > 8) nlen = 8;
        const uint64_t MAX_SCAN = (std::min)(m_phys_max, (uint64_t)BACKEND_SCAN_LIMIT);

        uint64_t found_cr3 = 0;
        uint64_t n_match=0, n_pidok=0, n_dtbok=0, n_mzok=0;


        static DWORD s_ep_pid = 0;
        static uint64_t s_ep_phys = 0;


        auto poll_ep_dtb = [&](uint64_t ep, int max_attempts) -> bool {
            uint32_t cand_off[32];
            uint64_t cand_pa[32];
            for (int attempt = 0; attempt < max_attempts; ++attempt) {
                if (!last_fcr3_stats.ep_dumped) {
                    bool all = true;
                    for (int i = 0; i < 0x40; i += 8) {
                        uint64_t v = 0;
                        if (!read_phys(ep + i, &v, 8)) { all = false; break; }
                        memcpy(last_fcr3_stats.ep_hdr + i, &v, 8);
                    }
                    if (all) last_fcr3_stats.ep_dumped = true;
                }

                size_t nc = 0;
                for (uint32_t o = 0; o + 8 <= 0x800 && nc < 32; o += 8) {
                    uint64_t raw = 0;
                    if (!read_phys(ep + o, &raw, 8)) { if (attempt == 0) last_fcr3_stats.readfail++; continue; }
                    if (raw >> 52) continue;
                    const uint64_t pa = raw & 0x000FFFFFFFFFF000ull;
                    if (pa < 0x10000 || pa >= m_phys_max) continue;
                    cand_off[nc] = o;
                    cand_pa[nc]  = pa;
                    nc++;
                }
                if (nc) n_dtbok++;
                for (size_t i = 0; i < nc; ++i) {
                    const uint64_t pa = cand_pa[i];
                    uint64_t kpml4e = 0;
                    if (!read_phys(pa + 0xFF8, &kpml4e, 8) || !(kpml4e & 1)) continue;
                    const uint64_t pdpt = kpml4e & 0x000FFFFFFFFFF000ull;
                    if (!pdpt || pdpt >= m_phys_max) continue;
                    if (m_peb) {
                        m_dtb = pa;
                        uint64_t base = read<uint64_t>(m_peb + 0x10);
                        uint64_t vpa = 0; uint16_t mz = 0;
                        bool ok = base && base >= 0x10000 && virt_to_phys(base, &vpa) && read_phys(vpa, &mz, 2) && mz == 0x5A4D;
                        m_dtb = 0;
                        if (!ok) continue;
                        n_mzok++;
                    }
                    if (verify_base) {
                        m_dtb = pa;
                        uint64_t vpa = 0; uint16_t mz = 0;
                        bool ok = virt_to_phys(verify_base, &vpa) && read_phys(vpa, &mz, 2) && mz == 0x5A4D;
                        m_dtb = 0;
                        if (!ok) continue;
                    }
                    found_cr3 = pa;
                    last_fcr3_stats.dtb_off = cand_off[i];
                    backend_cr3_cache_save(image, pid, pa, ep);
                    printf("[maxi] DTB found: EPROCESS+0x%X cr3=0x%llX (attempt=%d)\n",
                        cand_off[i], (unsigned long long)pa, attempt);
                    return true;
                }
                Sleep(15);
            }
            return false;
        };

        if (s_ep_phys && s_ep_pid == pid) {
            if (poll_ep_dtb(s_ep_phys, 4000)) {
                printf("[maxi] EP-cache poll HIT: ep=0x%llX cr3=0x%llX (full scan skipped)\n",
                    (unsigned long long)s_ep_phys, (unsigned long long)found_cr3);
                last_fcr3_stats.match = n_match; last_fcr3_stats.pidok = n_pidok;
                last_fcr3_stats.dtbok = n_dtbok; last_fcr3_stats.mzok = n_mzok;
                return found_cr3;
            }
        }


        auto try_ep = [&](uint64_t img_phys) -> bool {
            n_match++;
            if (img_phys < backend_img_off) return false;
            const uint64_t ep = img_phys - backend_img_off;
            uint64_t eppid = 0;
            if (!read_phys(ep + backend_pid_off, &eppid, 8)) return false;
            if ((uint32_t)eppid != pid) return false;
            n_pidok++;

            char fn[16] = {};
            bool name_ok = read_phys(ep + backend_img_off, fn, 15) && _strnicmp(fn, image, 15) == 0;
            if (name_ok) {
                const bool hit = poll_ep_dtb(ep, 4000);
                s_ep_pid = pid;
                s_ep_phys = ep;
                return hit;
            }
            if (poll_ep_dtb(ep, 130)) {
                printf("[maxi] PID+MZ match, name='%.15s' (Vanguard rename?) ep=0x%llX\n",
                    fn, (unsigned long long)ep);
                s_ep_pid = pid;
                s_ep_phys = ep;
                return true;
            }
            return false;
        };

        auto scan_buffer = [&](const uint8_t* buf, size_t n, uint64_t buf_phys) -> bool {
            for (size_t i = 0; i + nlen <= n; ) {
                const uint8_t* p = (const uint8_t*)memchr(buf + i, (int)(uint8_t)image[0], n - i);
                if (!p) break;
                const size_t pos = (size_t)(p - buf);
                if (pos + nlen > n) break;
                if (memcmp(p, image, nlen) != 0) { i = pos + 1; continue; }
                if (try_ep(buf_phys + pos)) return true;
                i = pos + 1;
            }
            return false;
        };





        const uint32_t peb_pml4_idx = (uint32_t)((m_peb >> 39) & 0x1FFull);
        auto scan_pml4_pages = [&](const uint8_t* buf, size_t n, uint64_t buf_phys) -> bool {
            if (!m_peb) return false;
            for (size_t off = 0; off + 0x1000 <= n; off += 0x1000) {
                const uint64_t* e = (const uint64_t*)(const void*)(buf + off);
                const uint64_t cand = buf_phys + off;

                if ((e[0x1FF] & 0x000FFFFFFFFFF000ull) != cand) continue;

                const uint64_t epbn = e[peb_pml4_idx];
                if (!(epbn & 1)) continue;
                const uint64_t epbn_pa = epbn & 0x000FFFFFFFFFF000ull;
                if (!epbn_pa || epbn_pa >= m_phys_max) continue;

                int present = 0;
                for (int i2 = 0; i2 < 0x100; ++i2) present += (int)(e[i2] & 1);
                if (present < 8) continue;
                m_dtb = cand;
                uint64_t base = read<uint64_t>(m_peb + 0x10);
                uint64_t vpa = 0; uint16_t mz = 0;
                bool ok = base && base >= 0x10000 &&
                          virt_to_phys(base, &vpa) && read_phys(vpa, &mz, 2) && mz == 0x5A4D;
                m_dtb = 0;
                if (!ok) continue;
                found_cr3 = cand;
                backend_cr3_cache_save(image, pid, cand, 0);
                printf("[maxi] PML4-DIRECT HIT: cr3=0x%llX (peb_idx=0x%X present=%d)\n",
                    (unsigned long long)cand, peb_pml4_idx, present);
                return true;
            }
            return false;
        };

        std::vector<uint8_t> wbuf(BACKEND_READ_WINDOW);
        const uint64_t CHUNK = 0x00400000ull;

        for (const auto& r : s_ranges) {
            const uint64_t end = (std::min)(r.end, MAX_SCAN);
            if (r.start >= end) continue;
            last_fcr3_stats.ranges++;
            if (!last_fcr3_stats.range_lo || r.start < last_fcr3_stats.range_lo) last_fcr3_stats.range_lo = r.start;
            if (end > last_fcr3_stats.range_hi) last_fcr3_stats.range_hi = end;
            for (uint64_t base = r.start; base < end; base += CHUNK) {
                const uint64_t wsz = (std::min)(CHUNK, end - base);
                last_fcr3_stats.scanned += wsz;
                if (!read_phys(base, wbuf.data(), wsz)) {
                    uint8_t b[4096];
                    for (uint64_t pg = base; pg < base + wsz; pg += 0x1000) {
                        if (!read_phys(pg, b, 4096)) continue;
                        if (scan_pml4_pages(b, 4096, pg)) break;
                        if (scan_buffer(b, 4096, pg)) break;
                    }
                    continue;
                }
                if (scan_pml4_pages(wbuf.data(), wsz, base)) break;
                if (scan_buffer(wbuf.data(), wsz, base)) break;
            }
            if (found_cr3) break;
        }
        last_fcr3_stats.match = n_match; last_fcr3_stats.pidok = n_pidok;
        last_fcr3_stats.dtbok = n_dtbok; last_fcr3_stats.mzok = n_mzok;
        printf("[fcr3] ozet: match=%llu pidok=%llu dtbok=%llu mzok=%llu\n", (unsigned long long)n_match, (unsigned long long)n_pidok, (unsigned long long)n_dtbok, (unsigned long long)n_mzok);
        return found_cr3;
    }


    struct backend_ptpage { uint64_t pa = ~0ull; uint64_t ts = 0; uint8_t data[4096]; };
    inline backend_ptpage* pt_cache() { static thread_local backend_ptpage c[4]; return c; }

    inline bool pt_entry(uint64_t dir_pa, size_t idx, uint64_t* out, int level) {
        if (level >= 0 && level < 4) {
            backend_ptpage& c = pt_cache()[level];
            const uint64_t now = GetTickCount64();
            const bool fresh = (level < 3) || (now - c.ts < 250);
            if (c.pa != dir_pa || !fresh) {
                if (!read_phys(dir_pa, c.data, 4096)) { c.pa = ~0ull; return false; }
                c.pa = dir_pa;
                c.ts = now;
            }
            *out = *(const uint64_t*)(const void*)(c.data + idx * 8);
            return true;
        }
        return read_phys(dir_pa + idx * 8, out, 8);
    }


    inline bool virt_to_phys(uint64_t va, uint64_t* pa) {
        if (!m_dtb) return false;
        uint64_t pml4e = 0, pdpte = 0, pde = 0, pte = 0, next = 0;
        if (!pt_entry(m_dtb, (va >> 39) & 0x1FF, &pml4e, 0)) return false;
        if (!(pml4e & 1)) return false;
        next = pml4e & 0x000FFFFFFFFFF000ull;
        if (!pt_entry(next, (va >> 30) & 0x1FF, &pdpte, 1)) return false;
        if (!(pdpte & 1)) return false;
        if (pdpte & 0x80) { *pa = (pdpte & 0x000FFFFFC0000000ull) + (va & 0x3FFFFFFFull); return true; }
        next = pdpte & 0x000FFFFFFFFFF000ull;
        if (!pt_entry(next, (va >> 21) & 0x1FF, &pde, 2)) return false;
        if (!(pde & 1)) return false;
        if (pde & 0x80) { *pa = (pde & 0x000FFFFFFFE00000ull) + (va & 0x1FFFFFull); return true; }
        next = pde & 0x000FFFFFFFFFF000ull;
        if (!pt_entry(next, (va >> 12) & 0x1FF, &pte, 3)) return false;
        if (!(pte & 1)) return false;
        *pa = (pte & 0x000FFFFFFFFFF000ull) + (va & 0xFFFull);
        return true;
    }




    template<typename T>
    inline std::optional<T> try_read(uint64_t address) {
        static_assert(std::is_trivially_copyable_v<T>, "try_read requires trivially copyable");
        uint64_t pa = 0;
        if (!virt_to_phys(address, &pa)) return std::nullopt;
        T out{};
        if (!read_phys(pa, &out, sizeof(T))) return std::nullopt;
        return out;
    }



    template<typename T>
    [[deprecated("Masks read failure as valid zero/false. Use try_read<T>() instead.")]]
    inline T read(uint64_t address) {
        return try_read<T>(address).value_or(T{});
    }

    inline bool read_memory(uint64_t address, void* buffer, size_t size) {
        size_t done = 0;
        while (done < size) {
            uint64_t pa = 0;
            if (!virt_to_phys(address + done, &pa)) return false;
            size_t chunk = (std::min)(size - done, BACKEND_PAGE_SIZE);
            if (!read_phys(pa, (uint8_t*)buffer + done, chunk)) return false;
            done += chunk;
        }
        return true;
    }

    inline bool read_batch(const uint64_t* entries, uint32_t count, void* out, size_t /*size*/) {
        if (!entries || !out) return false;
        uint8_t* dst = (uint8_t*)out;
        uint64_t last_page = ~0ull, last_pa = 0;
        for (uint32_t i = 0; i < count; ++i) {
            uint64_t pa = 0;
            const uint64_t page = entries[i] & ~0xFFFull;
            if (page == last_page)
                pa = last_pa + (entries[i] & 0xFFFull);
            else {
                if (!virt_to_phys(entries[i], &pa)) return false;
                last_page = page;
                last_pa = pa & ~0xFFFull;
            }
            if (!read_phys(pa, dst, sizeof(uintptr_t))) return false;
            dst += sizeof(uintptr_t);
        }
        return true;
    }
    inline bool read_batch(const batch_read_entry* entries, uint32_t count, void* out, size_t outSize) {
        if (!entries || !out) return false;
        uint8_t* dst = (uint8_t*)out;
        uint64_t last_page = ~0ull, last_pa = 0;
        for (uint32_t i = 0; i < count; ++i) {
            if (entries[i].offset + entries[i].size > outSize) return false;
            uint64_t pa = 0;
            const uint64_t page = entries[i].address & ~0xFFFull;
            if (page == last_page)
                pa = last_pa + (entries[i].address & 0xFFFull);
            else {
                if (!virt_to_phys(entries[i].address, &pa)) return false;
                last_page = page;
                last_pa = pa & ~0xFFFull;
            }
            if (!read_phys(pa, dst + entries[i].offset, entries[i].size)) return false;
        }
        return true;
    }

    inline bool is_valid(uint64_t addr) { return addr != 0; }

    inline bool move_mouse(int32_t /*dx*/, int32_t /*dy*/, int32_t /*dz*/ = 0) { return false; }
};

inline vm_t* g_vm = new vm_t();
