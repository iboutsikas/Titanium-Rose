#pragma once

#include "d3dx12.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"

#include "WinPixEventRuntime/pix3.h"

namespace Hazel {
    class D3D12CommandList {
    public:
        D3D12CommandList(TComPtr<ID3D12Device2> inDevice, TComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
        D3D12CommandList(TComPtr<ID3D12Device2> inDevice, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

        ~D3D12CommandList();

        HANDLE AddFenceEvent(TComPtr<ID3D12CommandQueue> commandQueue);
        uint64_t IncrementFence(TComPtr<ID3D12CommandQueue> commandQueue);


        void Close();
        void Reset(TComPtr<ID3D12CommandAllocator> commandAllocator);
        uint64_t Execute(TComPtr<ID3D12CommandQueue> commandQueue);
        void ExecuteAndWait(TComPtr<ID3D12CommandQueue> commandQueue);
        void Wait(TComPtr<ID3D12CommandQueue> commandQueue, uint64_t fenceValue = uint64_t(-1));

        inline void Track(TComPtr<ID3D12Resource>& resource) { m_TrackedResources.emplace_back(resource); };
        inline void Track(HeapAllocationDescription description) { m_TrackedAllocations.emplace_back(description); }
        inline void Track(std::vector<HeapAllocationDescription>& descriptions) { 
            m_TrackedAllocations.insert(std::end(m_TrackedAllocations), std::begin(descriptions), std::end(descriptions)); 
        }

        inline void MarkForExecution(bool shouldExecute = true) { m_ShouldExecute = true; }
        inline bool IsClosed() { return m_IsClosed; }
        inline bool ShouldExecute() { return m_ShouldExecute; }
        void ClearTracked();

        inline TComPtr<ID3D12GraphicsCommandList> Get() { return m_CommandList; }
        inline ID3D12GraphicsCommandList* GetRawPtr() { return m_CommandList.Get(); }

        void SetName(std::string name);
        inline void BeginEvent(const char* eventName, uint64_t color = PIX_COLOR_DEFAULT) const { PIXBeginEvent(m_CommandList.Get(), color, eventName); }
        inline void EndEvent() { PIXEndEvent(m_CommandList.Get()); }
    private:
        bool                                    m_IsClosed;
        bool                                    m_ShouldExecute;
        TComPtr<ID3D12Device2>                  m_Device;
        TComPtr<ID3D12GraphicsCommandList>      m_CommandList;
        TComPtr<ID3D12Fence>                    m_Fence;
        uint64_t                                m_FenceValue;
        std::vector<TComPtr<ID3D12Resource>>    m_TrackedResources;
        std::vector<HeapAllocationDescription>  m_TrackedAllocations;
        std::string m_Identifier;
    };
}
