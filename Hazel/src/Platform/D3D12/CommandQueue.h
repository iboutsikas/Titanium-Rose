#pragma once
#include <d3d12.h>

#include "Platform/D3D12/CommandAllocatorPool.h"

namespace Hazel
{
    class CommandQueue
    {
        // So we can call Execute
        friend class CommandListManager;
        friend class CommandContext;

    public:
        CommandQueue(D3D12_COMMAND_LIST_TYPE type);
        ~CommandQueue();

        void Initialize(ID3D12Device2* device);
        void Shutdown();

        inline bool IsInitialized() { return m_CommandQueue != nullptr; }
        
        uint64_t IncrementFence();
        bool IsFenceComplete(uint64_t FenceValue);

        /**
         * Stalls for a fence value that was signaled in another queue. For instance
         * you call this on the graphics queue to wait for a value that will be signaled in
         * the compute queue. 
         * 
         * The top 8 bits are used to represent the queue the value was signaled on.
         */
        void StallForFence(uint64_t FenceValue);

        /**
         * Stalls until the other queue is done with it's latest execution.
         */
        void StallForQueue(CommandQueue& other);

        /**
         * Waits for a value signaled on THIS queue.
         */
        void WaitForFence(uint64_t FenceValue);
        void WaitForIdle() { WaitForFence(IncrementFence()); }

        ID3D12CommandQueue* GetRawPtr() { return m_CommandQueue; }

    private:
        uint64_t ExecuteCommandList(ID3D12CommandList* list);
        ID3D12CommandAllocator* RequestAllocator();
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

    private:
        D3D12_COMMAND_LIST_TYPE m_Type;

        ID3D12CommandQueue* m_CommandQueue;

        CommandAllocatorPool m_AllocatorPool;

        ID3D12Fence*    m_Fence;
        uint64_t        m_FenceValue;
        uint64_t        m_LastCompletedFenceValue;
        HANDLE          m_FenceEvent;
    };

    class CommandListManager
    {
    public:

        CommandListManager();
        ~CommandListManager();

        inline CommandQueue& GetGraphicsQueue() { return m_GraphicsQueue; }
        inline CommandQueue& GetComputeQueue() { return m_ComputeQueue; }
        inline CommandQueue& GetCopyQueue() { return m_CopyQueue; }

        CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
        
        /**
         * Defaults to the graphics queue, as realistically this is the one that should be used most of the time.
         */
        ID3D12CommandQueue* GetCommandQueue() { return m_GraphicsQueue.GetRawPtr(); }

        void Initialize(ID3D12Device2* device);
        void Shutdown();

        void CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator);

        inline bool IsFenceComplete(uint64_t value) {
            return GetQueue(static_cast<D3D12_COMMAND_LIST_TYPE>(value >> 56)).IsFenceComplete(value);
        }

        inline void WaitForFence(uint64_t value);

        inline void WaitForIdle() {
            m_GraphicsQueue.WaitForIdle();
            m_ComputeQueue.WaitForIdle();
            m_CopyQueue.WaitForIdle();
        }

    private:
        ID3D12Device2* m_Device;

        CommandQueue m_GraphicsQueue;
        CommandQueue m_ComputeQueue;
        CommandQueue m_CopyQueue;
    };
}


