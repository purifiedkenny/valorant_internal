#pragma once









#include <memory>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>
#include <queue>
#include <stack>
#include <list>
#include <deque>


#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>


#include <chrono>


#include <functional>
#include <algorithm>
#include <ranges>
#include <concepts>


#include <iostream>
#include <fstream>
#include <sstream>
#include <format>


#include <utility>
#include <type_traits>
#include <optional>
#include <variant>
#include <any>
#include <exception>


#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>


#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif

#include <xorstr.hpp>
#include <nlohmann/json.hpp>






namespace maxi {

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;
    using f32 = float;
    using f64 = double;


    template<typename T>
    using vector = std::vector<T>;

    template<typename T>
    using shared_ptr = std::shared_ptr<T>;

    template<typename T>
    using unique_ptr = std::unique_ptr<T>;

    template<typename T>
    using atomic = std::atomic<T>;

    using string = std::string;
    using wstring = std::wstring;


    using thread = std::thread;
    using mutex = std::mutex;
    using condition_variable = std::condition_variable;
}


#define VEX_UNUSED(x) ((void)(x))
#define VEX_STRINGIFY(x) #x
#define VEX_TOSTRING(x) VEX_STRINGIFY(x)

#ifdef _DEBUG
    #define VEX_DEBUG_ONLY(x) x
    #define VEX_RELEASE_ONLY(x)
#else
    #define VEX_DEBUG_ONLY(x)
    #define VEX_RELEASE_ONLY(x) x
#endif


namespace maxi {
    namespace core {
        class Application;
    }

    namespace driver {
        class DriverImpl;
    }

    namespace game {
        class KernelProtocol;

        namespace model {
            class UWorld;
            class UObject;
        }

        namespace engine {
            class CoreEngine;
        }
    }

    namespace utils {
        class Logger;
        class ConfigManager;
    }
}
