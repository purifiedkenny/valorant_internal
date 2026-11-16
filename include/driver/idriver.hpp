#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <atomic>
#include <chrono>
#include <Windows.h>

namespace maxi::driver {



    inline std::atomic<uint64_t> g_read_calls{ 0 };
    inline std::atomic<uint64_t> g_read_bytes{ 0 };
    inline std::atomic<uint64_t> g_bone_transport_fail{ 0 };
    inline std::atomic<uint64_t> g_bone_garbage{ 0 };


    inline thread_local int g_read_cat = 0;
    inline std::atomic<uint64_t> g_cat_n[9]{};
    inline std::atomic<uint64_t> g_cat_bytes[9]{};
    inline std::atomic<uint64_t> g_cat_ns[9]{};
    inline std::atomic<uint64_t> g_cat_max[9]{};
    inline void cat_add(int cat, size_t bytes, uint64_t ns) {
        if (cat < 0 || cat > 8) cat = 0;
        g_cat_n[cat].fetch_add(1, std::memory_order_relaxed);
        g_cat_bytes[cat].fetch_add((uint64_t)bytes, std::memory_order_relaxed);
        g_cat_ns[cat].fetch_add(ns, std::memory_order_relaxed);
        uint64_t prev = g_cat_max[cat].load(std::memory_order_relaxed);
        while (ns > prev && !g_cat_max[cat].compare_exchange_weak(prev, ns, std::memory_order_relaxed)) {}
    }

    inline std::string target_process_name() {
        constexpr unsigned char enc[] = {
            0x0C, 0x1B, 0x16, 0x15, 0x08, 0x1B, 0x14, 0x0E, 0x77,
            0x0D, 0x33, 0x34, 0x6C, 0x6E, 0x77,
            0x09, 0x32, 0x33, 0x2A, 0x2A, 0x33, 0x34, 0x3D,
            0x74, 0x3F, 0x22, 0x3F
        };
        std::string s;
        s.reserve(sizeof(enc));
        for (unsigned char c : enc) s += static_cast<char>(c ^ 0x5A);
        return s;
    }

    inline std::wstring target_process_name_w() {
        std::string n = target_process_name();
        return std::wstring(n.begin(), n.end());
    }


    struct MemoryAllocation;
    struct ProcessInfo;
    struct ModuleInfo;

    





    class IDriver {
    public:
        IDriver() = default;
        virtual ~IDriver() = default;


        virtual bool setup() = 0;
        virtual void unload() = 0;
        virtual bool is_valid() const = 0;


        virtual bool attach_process(const std::wstring& process_name) = 0;
        virtual bool attach_process(uint32_t process_id) = 0;
        virtual uint32_t get_process_id() const = 0;
        virtual uintptr_t get_base_address() const = 0;
        virtual uintptr_t get_dtb() const = 0;



        virtual bool move_mouse(int32_t x, int32_t y, uint16_t button_flags) = 0;


        virtual bool read_memory(uintptr_t address, void* buffer, size_t size) = 0;


        virtual bool write_memory(void* dest, void* src, size_t size) = 0;

        virtual bool stream_mode(HWND hwnd, uint32_t flag) = 0;


        template<typename T>
        std::optional<T> try_read(uintptr_t address, size_t size = sizeof(T)) {
            ++g_read_calls;
            g_read_bytes += size;
            const auto t0 = std::chrono::steady_clock::now();
            T buffer{};
            const bool ok = read_memory(address, &buffer, size);
            const auto ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - t0).count();
            cat_add(g_read_cat, size, ns);
            if (ok) {
                return buffer;
            }
            return std::nullopt;
        }

        template<typename T>
        T read(uintptr_t address, size_t size = sizeof(T)) {
            return try_read<T>(address, size).value_or(T{});
        }

        template<typename T>
        bool write(uintptr_t address, const T& value) {
            return write_memory(reinterpret_cast<void*>(address), const_cast<T*>(&value), sizeof(T));
        }


        template<typename T>
        std::optional<std::vector<T>> try_read_array_batch(uintptr_t data_ptr, int32_t count) {
            if (!data_ptr || count <= 0) {
                return std::nullopt;
            }
            ++g_read_calls;
            const size_t total_size = count * sizeof(T);
            std::vector<std::uint8_t> buffer(total_size);

            g_read_bytes += total_size;
            const auto t0b = std::chrono::steady_clock::now();
            const bool bok = read_memory(data_ptr, buffer.data(), total_size);
            const auto nsb = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - t0b).count();
            cat_add(g_read_cat, total_size, nsb);
            if (!bok) {
                return std::nullopt;
            }

            std::vector<T> result;
            result.reserve(count);

            const T* data = reinterpret_cast<const T*>(buffer.data());
            for (int32_t i = 0; i < count; ++i) {
                result.emplace_back(data[i]);
            }

            return result;
        }

        template<typename T>
        [[deprecated("Masks read failure as empty vector. Use try_read_array_batch() instead.")]]
        std::vector<T> read_array_batch(uintptr_t data_ptr, int32_t count) {
            return try_read_array_batch<T>(data_ptr, count).value_or(std::vector<T>{});
        }


        virtual uintptr_t get_kernel_base(const std::string& module_name) = 0;


        virtual void* find_pattern(uintptr_t base, const char* pattern, const char* mask) = 0;

        virtual void set_dir_base(void* dir) = 0;
    };


    struct MemoryAllocation {
        uintptr_t address;
        uintptr_t mdl;
        size_t size;
        uint32_t protection;
    };

    struct ProcessInfo {
        uint32_t process_id;
        uintptr_t base_address;
        uintptr_t dtb;
        std::wstring name;
    };

    struct ModuleInfo {
        uintptr_t base_address;
        size_t size;
        std::string name;
    };

    


    std::shared_ptr<IDriver> create_driver();

}
