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

        inline TComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_CommandList; }
        inline TComPtr<ID3D12Device2> GetDevice() { return m_Device; }
        TComPtr<ID3D12GraphicsCommandList> Begin(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
        void Upload(_In_ ID3D12Resource* resource, uint32_t subresourceIndexStart,
            _In_reads_(numSubresources) const D3D12_SUBRESOURCE_DATA* subRes,
            uint32_t numSubresources);

        std::future<void> End(ID3D12CommandQueue* commandQueue);

    private:
        TComPtr<ID3D12Device2> m_Device;
        TComPtr<ID3D12GraphicsCommandList> m_CommandList;
        TComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        std::vector<TComPtr<ID3D12DeviceChild>> m_TrackedObjects;
        bool m_Finalized;

        struct Batch {
            std::vector<TComPtr<ID3D12DeviceChild>> TrackedObjects;
            TComPtr<ID3D12GraphicsCommandList>      CommandList;
            TComPtr<ID3D12Fence>  			        Fence;
            HANDLE                                  GpuCompleteEvent;

            Batch() noexcept : GpuCompleteEvent(nullptr) {}
        };
    };
}


