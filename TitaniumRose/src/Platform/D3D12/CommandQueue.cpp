#include "trpch.h"
#include "Platform/D3D12/CommandQueue.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Renderer.h"

static wchar_t* GetQueueName(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return L"GraphicsCommandQueue::";
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        return L"CommandCommandQueue::";
    case D3D12_COMMAND_LIST_TYPE_COPY:
        return L"CopyCommandQueue::";
    case D3D12_COMMAND_LIST_TYPE_BUNDLE:
    case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE:
    case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS:
    case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE:
        HZ_CORE_ASSERT(false, "Unsupported queue type");
        return L"UnsupportedCommandQueue::";
    default:
        HZ_CORE_ASSERT(false, "Unknown queue type");
        return L"UnknownCommandQueue::";
    }
}

namespace Roses {

    

    CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type)
        : m_Type(type), m_CommandQueue(nullptr), m_Fence(nullptr), m_AllocatorPool(type),
        m_FenceValue(static_cast<uint64_t>(type) << 56 | 1),
        m_LastCompletedFenceValue(static_cast<uint64_t>(type) << 56)
    {

    }

    CommandQueue::~CommandQueue()
    {
        Shutdown();
    }

    void CommandQueue::Initialize(ID3D12Device2* device)
    {
        HZ_CORE_ASSERT(device != nullptr, "Tried to create a command queue with a null device");
        HZ_CORE_ASSERT(IsInitialized() == false, "Command queue is already initialized");

        D3D12_COMMAND_QUEUE_DESC qDesc = {};
        qDesc.Type = m_Type;
        qDesc.NodeMask = 1;

        D3D12::ThrowIfFailed(device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_CommandQueue)));

        D3D12::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
        m_Fence->Signal(static_cast<uint64_t>(m_Type) << 56);

        std::wstringstream ss;
        ss << ::GetQueueName(m_Type) << TOSTRING(m_Fence);
        m_Fence->SetName(ss.str().c_str());

        m_FenceEvent = CreateEvent(nullptr, false, false, nullptr);
        HZ_CORE_ASSERT(m_FenceEvent != nullptr, "Fence event was null!");
        m_AllocatorPool.Initialize(device);

        HZ_CORE_ASSERT(m_CommandQueue != nullptr, "How did we get here and the command queue is still null?");
    }

    void CommandQueue::Shutdown()
    {
        if (m_CommandQueue == nullptr)
            return;

        ::CloseHandle(m_FenceEvent);
        m_Fence->Release();
        m_Fence = nullptr;

        m_CommandQueue->Release();
        m_CommandQueue = nullptr;
    }

    uint64_t CommandQueue::IncrementFence()
    {
        m_CommandQueue->Signal(m_Fence, m_FenceValue);
        return m_FenceValue++;
    }

    bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
//        HZ_CORE_ASSERT(m_LastCompletedFenceValue == m_Fence->GetCompletedValue(), "We have a shitty race condition");

        return fenceValue < m_LastCompletedFenceValue;
    }

    void CommandQueue::StallForFence(uint64_t fenceValue)
    {
        CommandQueue& other = D3D12Renderer::CommandQueueManager.GetQueue(static_cast<D3D12_COMMAND_LIST_TYPE>(fenceValue >> 56));
        m_CommandQueue->Wait(other.m_Fence, fenceValue);
    }

    void CommandQueue::StallForQueue(CommandQueue& other)
    {
        HZ_CORE_ASSERT(other.m_FenceValue > 0, "");
        m_CommandQueue->Wait(other.m_Fence, other.m_FenceValue - 1);
    }

    void CommandQueue::WaitForFence(uint64_t value)
    {
        if (IsFenceComplete(value))
            return;

        m_Fence->SetEventOnCompletion(value, m_FenceEvent);
        ::WaitForSingleObject(m_FenceEvent, INFINITE);
        m_LastCompletedFenceValue = value;
    }

    uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list)
    {
        D3D12::ThrowIfFailed(static_cast<ID3D12GraphicsCommandList*>(list)->Close());
        m_CommandQueue->ExecuteCommandLists(1, &list);
        m_CommandQueue->Signal(m_Fence, m_FenceValue);
        return m_FenceValue++;
    }

    ID3D12CommandAllocator* CommandQueue::RequestAllocator()
    {
        auto value = m_Fence->GetCompletedValue();
        return m_AllocatorPool.RequestAllocator(value);
    }

    void CommandQueue::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
    {
        m_AllocatorPool.DiscardAllocator(fenceValue, allocator);
    }



    /************************************************************************/
    /* Command List Manager                                                 */
    /************************************************************************/
    CommandListManager::CommandListManager()
        : m_Device(nullptr),
        m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
        m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
        m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
    {

    }

    CommandListManager::~CommandListManager()
    {
        Shutdown();
    }

    CommandQueue& CommandListManager::GetQueue(D3D12_COMMAND_LIST_TYPE type /*= D3D12_COMMAND_LIST_TYPE_DIRECT*/)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_GraphicsQueue;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
        default:
            HZ_CORE_ASSERT(false, "Unsupported queue type requested");
            return m_GraphicsQueue;
        }
    }

    void CommandListManager::Initialize(ID3D12Device2* device)
    {
        HZ_CORE_ASSERT(device != nullptr, "You got a null device there buddy");
        m_Device = device;

        m_GraphicsQueue.Initialize(device);
        m_ComputeQueue.Initialize(device);
        m_CopyQueue.Initialize(device);
    }

    void CommandListManager::Shutdown()
    {
        m_GraphicsQueue.Shutdown();
        m_ComputeQueue.Shutdown();
        m_CopyQueue.Shutdown();
    }

    void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            *allocator = m_GraphicsQueue.RequestAllocator();
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            *allocator = m_ComputeQueue.RequestAllocator();
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            *allocator = m_CopyQueue.RequestAllocator();
            break;
        default:
            HZ_CORE_ASSERT(false, "Unsupported command list type");
        }

        D3D12::ThrowIfFailed(m_Device->CreateCommandList(0, type, *allocator, nullptr, IID_PPV_ARGS(list)));
    }

    void CommandListManager::WaitForFence(uint64_t value)
    {
        CommandQueue& q = D3D12Renderer::CommandQueueManager.GetQueue(static_cast<D3D12_COMMAND_LIST_TYPE>(value >> 56));
        q.WaitForFence(value);
    }
}