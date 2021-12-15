#pragma once
#include <d3d12.h>
#include <queue>

namespace Roses {
    /**
     * A CommandAllocatorPool will handle creating new ID3D12CommandQueues when we need them
     * and will reuse them if their recorded commands have been executed.
     */
    class CommandAllocatorPool
    {
    public:
        CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);
        ~CommandAllocatorPool();

        void Initialize(ID3D12Device2* device);
        void Shutdown();

        ID3D12CommandAllocator* RequestAllocator(uint64_t fenceValue);
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

        inline size_t Size() { return m_AllAllocators.size(); }
    private:
        struct ReadyAllocator {
            uint64_t ReadyFenceValue;
            ID3D12CommandAllocator* Allocator;
        };
    private:
        D3D12_COMMAND_LIST_TYPE m_Type;

        ID3D12Device2* m_Device;
        std::vector<ID3D12CommandAllocator*> m_AllAllocators;
        std::queue<ReadyAllocator> m_ReadyQueue;
    };
}


