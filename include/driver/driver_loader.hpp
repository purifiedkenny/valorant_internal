#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

namespace maxi::driver::loader {

















    struct DriverLoadResult {
        bool success = false;
        std::wstring service_name;
        std::wstring temp_path;
        std::string error;
    };


    DriverLoadResult auto_load_driver();

    DriverLoadResult auto_load_aux_driver();


    bool auto_unload_driver(const std::wstring& service_name, const std::wstring& temp_path = L"");


    std::wstring generate_random_name(size_t length = 12);


    std::wstring get_hidden_drop_path();

}
