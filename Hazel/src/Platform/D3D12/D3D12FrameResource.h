#pragma once
#include "Platform/D3D12/ComPtr.h"
#include "d3d12.h"

namespace Hazel {
    struct D3D12FrameResource
    {
    public:

        D3D12FrameResource(TComPtr<ID3D12Device> device, UINT passCount);
        D3D12FrameResource(const D3D12FrameResource& rhs) = delete;
        D3D12FrameResource& operator=(const D3D12FrameResource& rhs) = delete;
        ~D3D12FrameResource();

        // We cannot reset the allocator until the GPU is done processing the commands.
        // So each frame needs their own allocator.
        TComPtr<ID3D12CommandAllocator> CommandAllocator;

        TComPtr<ID3D12CommandAllocator> CommandAllocator2;
        /// <summary>
        /// This will reset both command allocators. The callee needs to have
        /// waited for the fence before calling this function.
        /// </summary>
        void PrepareForNewFrame();

        // Fence value to mark commands up to this fence point.  This lets us
        // check if these frame resources are still in use by the GPU.
        UINT64 FenceValue = 0;
    };
}