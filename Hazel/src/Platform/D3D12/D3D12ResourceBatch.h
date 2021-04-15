#pragma once
#include <d3d12.h>
#include <future>
#include <stack>
#include "Hazel/Core/Image.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/Profiler/Profiler.h"
#include "Platform/D3D12/D3D12CommandList.h"


namespace Hazel {


    class D3D12ResourceBatch
    {
    public:
        explicit D3D12ResourceBatch(TComPtr<ID3D12Device2> device, Ref<D3D12CommandList> commandList, bool async = false) noexcept(false);
        
        ~D3D12ResourceBatch();

        inline TComPtr<ID3D12Device2> GetDevice() { return m_Device; }
        inline Ref<D3D12CommandList> GetCommandList() { return m_CommandList;  }

        Ref<D3D12CommandList> Begin(TComPtr<ID3D12CommandAllocator> commandAllocator);
        
        void Upload(ID3D12Resource* resource, uint32_t subresourceIndexStart,
            const D3D12_SUBRESOURCE_DATA* subRes,
            uint32_t numSubresources);

        void TrackResource(TComPtr<ID3D12Resource> resource);
        void TrackImage(Ref<Image> image);
        void TrackBlock(CPUProfileBlock& block);
        void TrackBlock(GPUProfileBlock& block);

        void End(TComPtr<ID3D12CommandQueue> commandQueue);
        void EndAndWait(TComPtr<ID3D12CommandQueue> commandQueue);
        std::future<void> EndAsync(TComPtr<ID3D12CommandQueue> commandQueue);

    private:
        TComPtr<ID3D12Device2>                      m_Device;
        Ref<D3D12CommandList>                       m_CommandList;

        std::vector<TComPtr<ID3D12DeviceChild>>     m_TrackedObjects;
        std::vector<Ref<Image>>                     m_TrackedImages;
        std::stack<CPUProfileBlock>                 m_CPUProfilingBlocks;
        std::stack<GPUProfileBlock>                 m_GPUProfilingBlocks;

        bool m_Finalized;
        bool m_IsAsync;

        struct Batch {
            std::vector<TComPtr<ID3D12DeviceChild>> TrackedObjects;
            std::vector<Ref<Image>>                 TrackedImages;
            HANDLE                                  GpuCompleteEvent;

            Batch() noexcept : GpuCompleteEvent(nullptr) {}
        };
    };
}


