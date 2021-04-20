#include "hzpch.h"
#include "CommandContext.h"

#include "Platform/D3D12/CommandQueue.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/GpuResource.h"

static Hazel::ContextManager s_ContextManager;

namespace Hazel {
    CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type)
        : m_Type(type), m_CommandList(nullptr), 
        m_CurrentAllocator(nullptr),
        m_NumCachedBarriers(0),
        m_CpuLinearAllocator(LinearAllocator::AllocatorType::CpuWritable),
        m_GpuLinearAllocator(LinearAllocator::AllocatorType::GpuExclusive)
    {
        ::memset(m_ResourceBarriers, 0, MAX_RESOURCE_BARRIERS * sizeof(D3D12_RESOURCE_BARRIER));
        ::memset(m_DescriptorHeaps, 0, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES * sizeof(ID3D12DescriptorHeap*));

    }

    CommandContext::~CommandContext()
    {
        if (m_CommandList != nullptr)
            m_CommandList->Release();
    }

    uint64_t CommandContext::Flush(bool waitForCompletion /*= false*/)
    {
        FlushResourceBarriers();

        HZ_CORE_ASSERT(m_CurrentAllocator != nullptr, "This context has not been properly reset/initialized");

        uint64_t fence = D3D12Renderer::CommandListManager.GetQueue(m_Type).ExecuteCommandList(m_CommandList);

        if (waitForCompletion)
            D3D12Renderer::CommandListManager.WaitForFence(fence);

        m_CommandList->Reset(m_CurrentAllocator, nullptr);

        // TODO: Reset root signature, pso, etc

        BindDescriptorHeaps();
        return fence;
    }

    uint64_t CommandContext::Finish(bool waitForCompletion /*= false*/)
    {
        HZ_CORE_ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE, "Invalid command list type");
        FlushResourceBarriers();

        // TODO: End profiling block here

        HZ_CORE_ASSERT(m_CurrentAllocator != nullptr, "");

        CommandQueue& queue = D3D12Renderer::CommandListManager.GetQueue(m_Type);

        auto fence = queue.ExecuteCommandList(m_CommandList);
        queue.DiscardAllocator(fence, m_CurrentAllocator);
        m_CurrentAllocator = nullptr;
        ReturnAllocations();
        if (waitForCompletion)
            D3D12Renderer::CommandListManager.WaitForFence(fence);

        s_ContextManager.FreeContext(this);
        
        return fence;
    }

    void CommandContext::Initialize()
    {
        D3D12Renderer::CommandListManager.CreateNewCommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
    }

    GraphicsContext& CommandContext::GetGraphicsContext()
    {
        HZ_CORE_ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT, "Not a graphics context");
        return reinterpret_cast<GraphicsContext&>(*this);
    }

    ComputeContext& CommandContext::GetComputeContext()
    {
        HZ_CORE_ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE, "Not a compute context");
        return reinterpret_cast<ComputeContext&>(*this);
    }

    void CommandContext::InitializeTexture(GpuResource& destination, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA subData[])
    {
        auto actualSize = GetRequiredIntermediateSize(destination.GetResource(), 0, numSubresources);

        CommandContext& ctx = CommandContext::Begin();

        auto mem = ctx.ReserveUploadMemory(actualSize);
        UpdateSubresources(ctx.m_CommandList, destination.GetResource(), mem.Buffer.GetResource(), 0, 0, numSubresources, subData);
        ctx.TransitionResource(destination, D3D12_RESOURCE_STATE_GENERIC_READ);
        ctx.Finish(true);
    }

    void CommandContext::InitializeBuffer(GpuResource& destination, const void* data, size_t sizeInBytes, size_t offset)
    {
        CommandContext& ctx = CommandContext::Begin();
        auto alloc = ctx.ReserveUploadMemory(sizeInBytes);

        ::memcpy(alloc.CpuAddress, data, sizeInBytes);

        ctx.TransitionResource(destination, D3D12_RESOURCE_STATE_COPY_DEST, true);
        ctx.m_CommandList->CopyBufferRegion(destination.GetResource(), offset, alloc.Buffer.GetResource(), 0, sizeInBytes);
        ctx.TransitionResource(destination, D3D12_RESOURCE_STATE_GENERIC_READ, true);
        ctx.Finish(true);
    }

    inline void CommandContext::CopyBuffer(GpuResource& source, GpuResource& destination)
    {
        TransitionResource(source, D3D12_RESOURCE_STATE_COPY_SOURCE);
        TransitionResource(destination, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();
        m_CommandList->CopyResource(destination.GetResource(), source.GetResource());
    }

    inline void CommandContext::CopyBufferRegion(GpuResource& destination, size_t offset, GpuResource& source, size_t srcOffset, size_t sizeInBytes)
    {
        TransitionResource(destination, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();
        m_CommandList->CopyBufferRegion(destination.GetResource(), offset, source.GetResource(), srcOffset, sizeInBytes);
    }

    void CommandContext::WriteBuffer(GpuResource& destination, size_t offset, const void* data, size_t sizeInBytes)
    {
        HZ_CORE_ASSERT(data != nullptr, "Data was null");

        DynamicAllocation alloc = m_CpuLinearAllocator.Allocate(sizeInBytes, 512);
        ::memcpy(alloc.CpuAddress, data, sizeInBytes);
        this->CopyBufferRegion(destination, offset, alloc.Buffer, alloc.Offset, sizeInBytes);
    }

    void CommandContext::FlushResourceBarriers()
    {
        if (m_NumCachedBarriers == 0)
            return;
        m_CommandList->ResourceBarrier(m_NumCachedBarriers, m_ResourceBarriers);
        m_NumCachedBarriers = 0;
    }

    void CommandContext::TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
    {
        D3D12_RESOURCE_STATES oldState = resource.m_CurrentState;

        if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE) 
        {
            HZ_CORE_ASSERT((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState, "");
            HZ_CORE_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState, "");
        }

        if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
            return InsertUAVBarrier(resource, flushImmediate);
        }

        if (oldState != newState) {
            HZ_CORE_ASSERT(m_NumCachedBarriers < MAX_RESOURCE_BARRIERS, "Run out of space for cached barriers. Flush sooner!");

            D3D12_RESOURCE_BARRIER& desc = m_ResourceBarriers[m_NumCachedBarriers++];
            desc.Transition.pResource = resource.GetResource();
            desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            desc.Transition.StateBefore = oldState;
            desc.Transition.StateAfter = newState;

            if (newState == resource.m_TransitioningState) {
                desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
                resource.m_TransitioningState = static_cast<D3D12_RESOURCE_STATES>(-1);
            }
            else {
                desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            }
            resource.m_CurrentState = newState;
        }

        if (flushImmediate || m_NumCachedBarriers == MAX_RESOURCE_BARRIERS) {
            FlushResourceBarriers();
        }
    }

    void CommandContext::InsertUAVBarrier(GpuResource& resource, bool flushImmediate)
    {
        HZ_CORE_ASSERT(m_NumCachedBarriers < MAX_RESOURCE_BARRIERS, "Run out of space for cached barriers. Flush sooner!");
        D3D12_RESOURCE_BARRIER& desc = m_ResourceBarriers[m_NumCachedBarriers++];

        desc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        desc.UAV.pResource = resource.GetResource();

        if (flushImmediate || m_NumCachedBarriers == MAX_RESOURCE_BARRIERS)
            FlushResourceBarriers();
    }

    CommandContext& CommandContext::Begin(const std::string name /*= ""*/)
    {
        CommandContext* ctx = s_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
        ctx->SetName(name);

        // TODO: Start profiling here
        return *ctx;
    }

    void CommandContext::BindDescriptorHeaps()
    {
        ID3D12DescriptorHeap* const heaps[] = {
            D3D12Renderer::s_ResourceDescriptorHeap->GetHeap()
        };

        m_CommandList->SetDescriptorHeaps(1, heaps);
#if 0
        uint32_t numHeaps = 0;
        ID3D12DescriptorHeap* heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
        for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
        {
            if (m_DescriptorHeaps[i] != nullptr) {
                heaps[numHeaps++] = m_DescriptorHeaps[i];
            }
        }

        if (numHeaps > 0) {
            m_CommandList->SetDescriptorHeaps(numHeaps, heaps);
        }
#endif
    }

    void CommandContext::ReturnAllocations() {
        for (auto& allocation : m_RTVAllocations)
        {
            D3D12Renderer::s_RenderTargetDescriptorHeap->Release(allocation);
        }

        for (auto& allocation : m_ShaderAllocations)
        {
            D3D12Renderer::s_ResourceDescriptorHeap->Release(allocation);
        }
    }

    void CommandContext::Reset()
    {
        HZ_CORE_ASSERT(m_CommandList != nullptr || m_CurrentAllocator == nullptr, "Allocator was not propery released before reset");
        m_CurrentAllocator = D3D12Renderer::CommandListManager.GetQueue(m_Type).RequestAllocator();
        m_CommandList->Reset(m_CurrentAllocator, nullptr);

        m_NumCachedBarriers = 0;
        BindDescriptorHeaps();
    }

    CommandContext* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type)
    {
        auto& contextsForType = m_AvailableContexts[type];

        CommandContext* ctx = nullptr;

        if (contextsForType.empty()) {
            ctx = new CommandContext(type);
            m_ContextPool[type].emplace_back(ctx);
            ctx->Initialize();
        }
        else {
            ctx = contextsForType.front();
            contextsForType.pop();
            ctx->Reset();
        }

        HZ_CORE_ASSERT(ctx != nullptr, "The context we got is null");
        HZ_CORE_ASSERT(ctx->m_Type == type, "Something's gone horrible wrong with the queues, we got a contex for the wrong type");

        return ctx;
    }

    void ContextManager::FreeContext(CommandContext* context)
    {
        HZ_CORE_ASSERT(context != nullptr, "Called with null context");

        m_AvailableContexts[context->m_Type].push(context);
    }

    void ContextManager::DestroyAll()
    {
        for (auto& contextsForType : m_ContextPool) {
            for (int i = 0; i < contextsForType.size(); i++) {
                delete contextsForType[i];
            }
            contextsForType.clear();
        }
    }

    ComputeContext& ComputeContext::Begin(const std::string& name, bool async)
    {
        auto type = async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT;
        ComputeContext& context = s_ContextManager.AllocateContext(type)->GetComputeContext();
        context.SetName(name);
        // TODO: Start profiling here

        return context;
    }

    void ComputeContext::SetShader(D3D12Shader& shader)
    {
        HZ_CORE_ASSERT(shader.ContainsShader(ShaderType::Compute), "Shader is not a compute shader");
        m_CommandList->SetComputeRootSignature(shader.GetRootSignature());
        m_CommandList->SetPipelineState(shader.GetPipelineState());
    }

    inline void ComputeContext::SetDynamicContantBufferView(uint32_t rootIndex, size_t sizeInBytes, const void* data)
    {
        HZ_CORE_ASSERT(data != nullptr, "");

        DynamicAllocation alloc = m_CpuLinearAllocator.Allocate(sizeInBytes);
        ::memcpy(alloc.CpuAddress, data, sizeInBytes);
        m_CommandList->SetComputeRootConstantBufferView(rootIndex, alloc.GpuAddress);
    }

    inline void GraphicsContext::SetDynamicContantBufferView(uint32_t rootIndex, size_t sizeInBytes, const void* data)
    {
        HZ_CORE_ASSERT(data != nullptr, "");

        DynamicAllocation alloc = m_CpuLinearAllocator.Allocate(sizeInBytes);
        ::memcpy(alloc.CpuAddress, data, sizeInBytes);
        m_CommandList->SetGraphicsRootConstantBufferView(rootIndex, alloc.GpuAddress);
    }

}