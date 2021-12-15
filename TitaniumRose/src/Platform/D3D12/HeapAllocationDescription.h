#pragma once
#include "d3d12.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Roses {
    struct HeapAllocationDescription
    {
    public:
        HeapAllocationDescription()
            : Allocated(false), OffsetInHeap(0), Range(0),
            CPUHandle({ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN }),
            GPUHandle({ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN })
        {}
        bool   Allocated;
        size_t OffsetInHeap;
        size_t Range;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
    };
}
