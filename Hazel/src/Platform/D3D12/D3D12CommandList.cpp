#include "hzpch.h"
#include "Platform/D3D12/D3D12CommandList.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Hazel {
    D3D12CommandList::D3D12CommandList(TComPtr<ID3D12Device2> inDevice, TComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
        : m_IsClosed(true), m_ShouldExecute(false), m_Device(inDevice), m_CommandList(nullptr), m_FenceValue(0)
    {
        //HZ_ERROR("D3D12CommandList::D3D12CommandList()");
        D3D12::ThrowIfFailed(inDevice->CreateCommandList(
            0, 
            type, 
            commandAllocator.Get(), 
            nullptr, 
            IID_PPV_ARGS(&m_CommandList))
        );

        D3D12::ThrowIfFailed(inDevice->CreateFence(
            0, D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&m_Fence)
        ));
    }

    D3D12CommandList::D3D12CommandList(TComPtr<ID3D12Device2> inDevice, D3D12_COMMAND_LIST_TYPE type)
        : m_IsClosed(true), m_ShouldExecute(false), m_Device(inDevice), m_CommandList(nullptr), m_FenceValue(0)
    {
        TComPtr<ID3D12Device4> dev;

        D3D12::ThrowIfFailed(inDevice->QueryInterface(IID_PPV_ARGS(&dev)));

        D3D12::ThrowIfFailed(dev->CreateCommandList1(
            0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_CommandList)
        ));

        D3D12::ThrowIfFailed(inDevice->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)
        ));
    }

    D3D12CommandList::~D3D12CommandList()
    {
        //HZ_ERROR("D3D12CommandList::~D3D12CommandList()");
    }

    void D3D12CommandList::Reset(TComPtr<ID3D12CommandAllocator> commandAllocator)
    {
        D3D12::ThrowIfFailed(m_CommandList->Reset(commandAllocator.Get(), nullptr));
        m_IsClosed = false;
    }

    void D3D12CommandList::Close()
    {
        D3D12::ThrowIfFailed(m_CommandList->Close());
        m_IsClosed = true;
    }

    void D3D12CommandList::Execute(TComPtr<ID3D12CommandQueue> commandQueue)
    {
        this->Close();

        ID3D12CommandList* const commandLists[] = {
            m_CommandList.Get()
        };

        commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    }

    void D3D12CommandList::ExecuteAndWait(TComPtr<ID3D12CommandQueue> commandQueue)
    {
        this->Execute(commandQueue);

        
        m_FenceValue++;
        auto completedValue = m_Fence->GetCompletedValue();
        
        D3D12::ThrowIfFailed(commandQueue->Signal(m_Fence.Get(), m_FenceValue));
        if (completedValue < m_FenceValue) {
            HANDLE evt = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            HZ_CORE_ASSERT(evt, "Fence event was null");

            D3D12::ThrowIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, evt));

            ::WaitForSingleObject(evt, INFINITE);
#if HZ_DEBUG
            auto dbgValue = m_Fence->GetCompletedValue();
            HZ_CORE_ASSERT(dbgValue >= m_FenceValue, "Expected fence value to be greater than the current one");
#endif
            CloseHandle(evt);
        }
    }
        
    HANDLE D3D12CommandList::AddFenceEvent(TComPtr<ID3D12CommandQueue> commandQueue)
    {
        m_FenceValue++;
        D3D12::ThrowIfFailed(commandQueue->Signal(m_Fence.Get(), m_FenceValue));
        HANDLE evt = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        HZ_CORE_ASSERT(evt, "Fence event was null");
        D3D12::ThrowIfFailed(m_Fence->SetEventOnCompletion(m_FenceValue, evt));
        return evt;
    }

    void D3D12CommandList::SetName(std::string name)
    {
        m_Identifier = name;

        std::wstring wName(m_Identifier.begin(), m_Identifier.end());

        m_CommandList->SetName(wName.c_str());
    }
}
