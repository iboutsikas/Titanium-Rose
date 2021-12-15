#include "trpch.h"
#include "CommandAllocatorPool.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Roses {
    CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type)
        : m_Device(nullptr),
        m_Type(type)
    {

    }

    CommandAllocatorPool::~CommandAllocatorPool()
    {
        Shutdown();
    }

    void CommandAllocatorPool::Initialize(ID3D12Device2* device)
    {
        m_Device = device;
    }

    void CommandAllocatorPool::Shutdown()
    {
        for (uint64_t i = 0; i < m_AllAllocators.size(); i++ )
        {
            m_AllAllocators[i]->Release();
        }
        m_AllAllocators.clear();
    }

    ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t fenceValue)
    {
        ID3D12CommandAllocator* allocator = nullptr;

        if (!m_ReadyQueue.empty()) {

            auto& readyAllocator = m_ReadyQueue.front();

            if (readyAllocator.ReadyFenceValue <= fenceValue) {
                allocator = readyAllocator.Allocator;
                D3D12::ThrowIfFailed(allocator->Reset());
                m_ReadyQueue.pop();
            }
        }

        if (allocator == nullptr) {
            D3D12::ThrowIfFailed(m_Device->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&allocator)));
            m_AllAllocators.push_back(allocator);
        }

        return allocator;
    }

    void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
    {
        m_ReadyQueue.push({ fenceValue, allocator });
    }
}

