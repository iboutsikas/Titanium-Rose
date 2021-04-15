#pragma once

#include "d3dx12.h"

#include "Platform/D3D12/ComPtr.h"
#include "WinPixEventRuntime/pix3.h"

namespace Hazel {
    typedef HANDLE;

    class D3D12CommandList {
    public:
        D3D12CommandList(TComPtr<ID3D12Device2> inDevice, TComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
        D3D12CommandList(TComPtr<ID3D12Device2> inDevice, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

        ~D3D12CommandList();

        void Close();
        void Execute(TComPtr<ID3D12CommandQueue> commandQueue);
        void ExecuteAndWait(TComPtr<ID3D12CommandQueue> commandQueue);
        void Reset(TComPtr<ID3D12CommandAllocator> commandAllocator);

        HANDLE AddFenceEvent(TComPtr<ID3D12CommandQueue> commandQueue);
        
        inline void MarkForExecution(bool shouldExecute = true) { m_ShouldExecute = true; }
        inline bool IsClosed() { return m_IsClosed; }
        inline bool ShouldExecute() { return m_ShouldExecute; }


        inline TComPtr<ID3D12GraphicsCommandList> Get() { return m_CommandList; }
        inline ID3D12GraphicsCommandList* GetRawPtr() { return m_CommandList.Get(); }

        void SetName(std::string name);
        inline void BeginEvent(const char* eventName, uint64_t color = PIX_COLOR_DEFAULT) const { PIXBeginEvent(m_CommandList.Get(), color, eventName); }
        inline void EndEvent() { PIXEndEvent(m_CommandList.Get()); }
    private:
        bool                                m_IsClosed;
        bool                                m_ShouldExecute;
        TComPtr<ID3D12Device2>              m_Device;
        TComPtr<ID3D12GraphicsCommandList>  m_CommandList;
        TComPtr<ID3D12Fence>                m_Fence;
        uint64_t                            m_FenceValue;
        std::string m_Identifier;
    };
}
