#include "../../include/common.hpp"
#include "../../include/driver/driver_context.hpp"
#include "../../include/game/kernel_protocol.hpp"
#include "../../include/game/memory_map.hpp"
#include "../../include/utils/logger.hpp"
#include "../../include/driver/idriver.hpp"
#include <intrin.h>

namespace maxi::game {
    using namespace maxi::utils;


    static std::string vgk_name() {

        static const uint8_t e[] = { 0x5C,0x42,0x4F,0x2E,0x18,0x1E,0x1F };
        std::string s(7, '\0');
        for (int i = 0; i < 7; ++i) s[i] = static_cast<char>(e[i] ^ 0x2B);
        return s;
    }

	uintptr_t KernelProtocol::derive_cloned_cr3_simplified(uintptr_t cloned_cr3)
	{
		uintptr_t Last = cloned_cr3 & 0xFFFFFFFFF;
		uintptr_t FirstDigit = Last >> 32 & 0xF;
		uintptr_t LastDigit = Last & 0xF;

		FirstDigit -= 1;
		LastDigit -= 1;

		return FirstDigit << 32 | Last & 0x0FFFFFFF0 | LastDigit;
	}

	ShadowData KernelProtocol::find_pml4_base() {
		ShadowRegionsDataStructure data = maxi::driver::g_driver->read<ShadowRegionsDataStructure>(maxi::driver::g_driver->get_kernel_base(vgk_name()) + memory_map::vgx::ShadowRegions);
		if (!data.OriginalPML4_t || !data.ClonedPML4_t) {
			data = maxi::driver::g_driver->read<ShadowRegionsDataStructure>(maxi::driver::g_driver->get_kernel_base(vgk_name()) + memory_map::old_vgx::ShadowRegions);
		}
		auto decypted_cloned_cr3 = this->derive();
		return { decypted_cloned_cr3, data.FreeIndex << 0x27 };
	}

	uint64_t KernelProtocol::cpuid_rax() {
		int cpuInfo[4] = { 0 };
		__cpuid(cpuInfo, 1);
		return static_cast<int64_t>(static_cast<int32_t>(cpuInfo[0]));
	}

	uintptr_t KernelProtocol::derive() {
		try
		{
			auto base = maxi::driver::g_driver->get_kernel_base(vgk_name());
			if(!base) {
				return 0;
			}

			auto byte_opt = maxi::driver::g_driver->try_read<uint8_t>(base + memory_map::vgx::ShadowRegionB);
			uint8_t byte = byte_opt.value_or(0);
			if(!byte) {
				byte = maxi::driver::g_driver->try_read<uint8_t>(base + memory_map::old_vgx::ShadowRegionB).value_or(0);
			}
			auto key_opt = maxi::driver::g_driver->try_read<uintptr_t>(base + memory_map::vgx::ShadowRegionQ);
			uintptr_t key = key_opt.value_or(0);
			if(!key) {
				key = maxi::driver::g_driver->try_read<uintptr_t>(base + memory_map::old_vgx::ShadowRegionQ).value_or(0);
			}

			auto _RAX = cpuid_rax();
			uint64_t v5 = byte & memory_map::derive::ByteMask;
			uint64_t v11 = ~(int)_RAX;

			uint64_t v12 =
				(memory_map::derive::VgkK1 * (int)_RAX
					+ memory_map::derive::VgkK2 * ~key
					+ memory_map::derive::VgkK3 * (key + 1)
					+ (((key ^ (int)_RAX) << 63) ^ memory_map::derive::VgkK4))
				*
				(memory_map::derive::VgkK5 * key * (memory_map::derive::VgkK6 * key + memory_map::derive::VgkK7)
					+ memory_map::derive::VgkK8 * ~(key * (memory_map::derive::VgkK6 * key + memory_map::derive::VgkK7))
					+ (key *
						(memory_map::derive::VgkK9 * ~key
							+ memory_map::derive::VgkK10 * (int)_RAX
							+ memory_map::derive::VgkK11 * v11
							+ memory_map::derive::VgkK12 *
							(
								((int)_RAX | memory_map::derive::VgkK13) -
								(memory_map::derive::VgkK13 - 1 - ((int)_RAX & memory_map::derive::VgkK13))
								)
							)
						+ memory_map::derive::VgkK14)
					*
					(memory_map::derive::VgkK15 * (int)_RAX
						+ memory_map::derive::VgkK16 * (v5 | key)
						+ memory_map::derive::VgkK17 * v11
						+ memory_map::derive::VgkK18 * (~key & ~v5)
						+ ((key + (v5 | (int)_RAX) + (v5 & (key ^ (int)_RAX))) << 63))
					+ memory_map::derive::VgkK19);

			uint64_t expr =
				memory_map::derive::VgkK20 * key +
				memory_map::derive::VgkK21 * ~v5 +
				memory_map::derive::VgkK22 * v5 +
				memory_map::derive::VgkK23 * ((v5 & key) - (key | ~v5));

			uint64_t logic =
				-3 * ~key +
				(v5 ^ key) +
				-2 * (v5 ^ (v5 | key)) +
				2 * (((v5 | memory_map::derive::VgkK13) ^ memory_map::derive::VgkK24) + (~key | v5 ^ memory_map::derive::VgkK13)) -
				(v5 ^ ~key ^ memory_map::derive::VgkK13) -
				memory_map::derive::VgkK13;

			uint64_t v13 =
				((~(expr * logic) ^ ~v12) << 63) +
				memory_map::derive::VgkK25 *
				(
					((int)_RAX & (expr * logic)) -
					(v11 & ~(expr * logic)) -
					(int)_RAX
					);

			uint64_t DecryptedCR3 =
				memory_map::derive::VgkK26 * (expr * logic) +
				v13 -
				memory_map::derive::VgkK28 * v12 -
				memory_map::derive::VgkK27;

			return DecryptedCR3;
		}
		catch (...) {
		}
	}

}
