#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <windows.h>

namespace maxi::driver::crypt {






    constexpr uint8_t XOR_KEY[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0x37, 0x13, 0x42, 0x69, 0xF0, 0x0D, 0xC0, 0xDE
    };
    constexpr size_t XOR_KEY_LEN = sizeof(XOR_KEY);





    inline void xor_transform(uint8_t* data, size_t size) {
        for (size_t i = 0; i < size; ++i) {

            data[i] ^= XOR_KEY[i % XOR_KEY_LEN];


        }
    }



    inline void rotate_encrypt(uint8_t* data, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            uint8_t rot = static_cast<uint8_t>((i % 7) + 1);
            data[i] = static_cast<uint8_t>((data[i] << rot) | (data[i] >> (8 - rot)));
        }
    }

    inline void rotate_decrypt(uint8_t* data, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            uint8_t rot = static_cast<uint8_t>((i % 7) + 1);
            data[i] = static_cast<uint8_t>((data[i] >> rot) | (data[i] << (8 - rot)));
        }
    }





    inline void full_encrypt(uint8_t* data, size_t size) {
        xor_transform(data, size);
        rotate_encrypt(data, size);
    }

    inline void full_decrypt(uint8_t* data, size_t size) {
        rotate_decrypt(data, size);
        xor_transform(data, size);
    }






    std::vector<uint8_t> load_and_decrypt_file(const std::wstring& path);


    std::vector<uint8_t> load_and_decrypt_resource(HMODULE hModule, int resourceId);

}
