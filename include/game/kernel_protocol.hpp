#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "../driver/idriver.hpp"

namespace maxi::game {

struct ShadowData
{
    uintptr_t DecryptedClonedCr3;
    uintptr_t PML4Base;
};

typedef struct ShadowRegionsDataStructure
{
    uintptr_t OriginalPML4_t;
    uintptr_t ClonedPML4_t;
    uintptr_t GameCr3;
    uintptr_t ClonedCr3;
    uintptr_t FreeIndex;
} ShadowRegionsDataStructure;

class KernelProtocol
{
private:
    ShadowData m_shadow_data;
    ShadowRegionsDataStructure m_shadow_regions_data;

    uint64_t cpuid_rax();

    uintptr_t derive();

    uintptr_t derive_cloned_cr3_simplified(uintptr_t cloned_cr3);
public:
    KernelProtocol() {
    }

    ~KernelProtocol() = default;

    ShadowData find_pml4_base();
};

inline std::shared_ptr<KernelProtocol> m_kernel_protocol = std::make_shared<KernelProtocol>();

}
