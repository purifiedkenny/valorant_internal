#pragma once













#include <cstdint>

namespace maxi::build {





    constexpr uint32_t compile_time_seed() {

        constexpr const char* t = __TIME__;
        uint32_t seed = 0;
        seed  = static_cast<uint32_t>(t[0] - '0') * 100000;
        seed += static_cast<uint32_t>(t[1] - '0') * 10000;
        seed += static_cast<uint32_t>(t[3] - '0') * 1000;
        seed += static_cast<uint32_t>(t[4] - '0') * 100;
        seed += static_cast<uint32_t>(t[6] - '0') * 10;
        seed += static_cast<uint32_t>(t[7] - '0');
        return seed ^ 0xDEADBEEF;
    }

    constexpr uint32_t compile_time_date_hash() {
        constexpr const char* d = __DATE__;
        uint32_t hash = 5381;
        for (int i = 0; d[i] != '\0'; ++i) {
            hash = ((hash << 5) + hash) + static_cast<uint8_t>(d[i]);
        }
        return hash;
    }


    constexpr uint32_t BUILD_STAMP = compile_time_seed() ^ compile_time_date_hash();





    constexpr uint8_t poly_pad_0 = static_cast<uint8_t>((BUILD_STAMP >> 0) & 0xFF);
    constexpr uint8_t poly_pad_1 = static_cast<uint8_t>((BUILD_STAMP >> 8) & 0xFF);
    constexpr uint8_t poly_pad_2 = static_cast<uint8_t>((BUILD_STAMP >> 16) & 0xFF);
    constexpr uint8_t poly_pad_3 = static_cast<uint8_t>((BUILD_STAMP >> 24) & 0xFF);


    constexpr uint8_t POLY_BLOCK[64] = {
        poly_pad_0, poly_pad_1, poly_pad_2, poly_pad_3,
        static_cast<uint8_t>(poly_pad_0 ^ 0xAA), static_cast<uint8_t>(poly_pad_1 ^ 0x55),
        static_cast<uint8_t>(poly_pad_2 ^ 0xCC), static_cast<uint8_t>(poly_pad_3 ^ 0x33),
        static_cast<uint8_t>(poly_pad_0 + poly_pad_1), static_cast<uint8_t>(poly_pad_2 - poly_pad_3),
        static_cast<uint8_t>(poly_pad_0 * 3), static_cast<uint8_t>(poly_pad_1 * 7),
        static_cast<uint8_t>(poly_pad_2 * 11), static_cast<uint8_t>(poly_pad_3 * 13),
        static_cast<uint8_t>(poly_pad_0 ^ poly_pad_2), static_cast<uint8_t>(poly_pad_1 ^ poly_pad_3),

        static_cast<uint8_t>((poly_pad_0 << 1) | (poly_pad_0 >> 7)),
        static_cast<uint8_t>((poly_pad_1 << 2) | (poly_pad_1 >> 6)),
        static_cast<uint8_t>((poly_pad_2 << 3) | (poly_pad_2 >> 5)),
        static_cast<uint8_t>((poly_pad_3 << 4) | (poly_pad_3 >> 4)),
        static_cast<uint8_t>((poly_pad_0 << 5) | (poly_pad_0 >> 3)),
        static_cast<uint8_t>((poly_pad_1 << 6) | (poly_pad_1 >> 2)),
        static_cast<uint8_t>((poly_pad_2 << 7) | (poly_pad_2 >> 1)),
        static_cast<uint8_t>(poly_pad_3 ^ 0xFF),
        static_cast<uint8_t>(poly_pad_0 + 0x37), static_cast<uint8_t>(poly_pad_1 + 0x13),
        static_cast<uint8_t>(poly_pad_2 + 0x42), static_cast<uint8_t>(poly_pad_3 + 0x69),
        static_cast<uint8_t>(poly_pad_0 - 0x11), static_cast<uint8_t>(poly_pad_1 - 0x22),
        static_cast<uint8_t>(poly_pad_2 - 0x33), static_cast<uint8_t>(poly_pad_3 - 0x44),

        static_cast<uint8_t>(poly_pad_0 ^ poly_pad_1 ^ poly_pad_2),
        static_cast<uint8_t>(poly_pad_1 ^ poly_pad_2 ^ poly_pad_3),
        static_cast<uint8_t>(poly_pad_0 ^ poly_pad_1 ^ poly_pad_3),
        static_cast<uint8_t>(poly_pad_0 ^ poly_pad_2 ^ poly_pad_3),
        static_cast<uint8_t>(poly_pad_0 + poly_pad_1 + poly_pad_2),
        static_cast<uint8_t>(poly_pad_1 + poly_pad_2 + poly_pad_3),
        static_cast<uint8_t>((BUILD_STAMP >> 4) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP >> 12) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP >> 20) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP >> 28) & 0x0F),
        static_cast<uint8_t>((BUILD_STAMP * 7) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP * 13) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP * 17) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP * 23) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP * 29) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP * 31) & 0xFF),

        static_cast<uint8_t>(poly_pad_0 & poly_pad_1),
        static_cast<uint8_t>(poly_pad_2 | poly_pad_3),
        static_cast<uint8_t>(~poly_pad_0),
        static_cast<uint8_t>(~poly_pad_1),
        static_cast<uint8_t>(~poly_pad_2),
        static_cast<uint8_t>(~poly_pad_3),
        static_cast<uint8_t>((poly_pad_0 * poly_pad_1) & 0xFF),
        static_cast<uint8_t>((poly_pad_2 * poly_pad_3) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP + 0xCAFEBABE) & 0xFF),
        static_cast<uint8_t>(((BUILD_STAMP + 0xCAFEBABE) >> 8) & 0xFF),
        static_cast<uint8_t>(((BUILD_STAMP + 0xCAFEBABE) >> 16) & 0xFF),
        static_cast<uint8_t>(((BUILD_STAMP + 0xCAFEBABE) >> 24) & 0xFF),
        static_cast<uint8_t>((BUILD_STAMP ^ 0x13371337) & 0xFF),
        static_cast<uint8_t>(((BUILD_STAMP ^ 0x13371337) >> 8) & 0xFF),
        static_cast<uint8_t>(((BUILD_STAMP ^ 0x13371337) >> 16) & 0xFF),
        static_cast<uint8_t>(((BUILD_STAMP ^ 0x13371337) >> 24) & 0xFF),
    };





    inline volatile uint8_t __poly_anchor = 0;

    inline void anchor_polymorphic_data() {

        volatile uint8_t sink = 0;
        for (int i = 0; i < 64; ++i) {
            sink ^= POLY_BLOCK[i];
        }
        __poly_anchor = sink;
    }

}
