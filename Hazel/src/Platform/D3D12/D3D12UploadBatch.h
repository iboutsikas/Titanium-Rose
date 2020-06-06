#pragma once
#include "d3d12.h"
#include "future"

#include "Platform/D3D12/ComPtr.h"

namespace Hazel {
    class D3D12ResourceUploadBatch
    {
    public:
        D3D12ResourceUploadBatch(TComPtr<ID3D12Device2> device);
        ~D3D12ResourceUploadBatch();

        TComPtr<ID3D12GraphicsCommandList> Begin(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
        std::future<void> End(ID3D12CommandQueue* commandQueue);

    private:
        TComPtr<ID3D12Device2> m_Device;
        TComPtr<ID3D12GraphicsCommandList> m_CommandList;
        TComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        bool m_Finalized;

        struct Batch {
            TComPtr<ID3D12GraphicsCommandList>      CommandList;
            TComPtr<ID3D12Fence>  			        Fence;
            HANDLE                                  GpuCompleteEvent;

            Batch() noexcept : GpuCompleteEvent(nullptr) {}
        };
    };
}


