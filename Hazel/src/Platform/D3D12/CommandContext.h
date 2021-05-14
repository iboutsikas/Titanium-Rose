#pragma once
#include <d3d12.h>

#include "Platform/D3D12/GpuResource.h"
#include "Platform/D3D12/LinearAllocator.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"

#include "Platform/D3D12/CommandQueue.h"

namespace Hazel {
    class CommandListManager;
    class GraphicsContext;
    class ComputeContext;
    class Texture2D;

    struct Param32 {
        Param32(float f)    : Float32(f) {}
        Param32(int32_t i)  : Int32(i) {}
        Param32(uint32_t u) : Uint32(u) {}
        
        void operator=(float f) { Float32 = f; }
        void operator=(int32_t i) { Int32 = i; }
        void operator=(uint32_t u) { Uint32 = u; }

        union 
        {
            float    Float32;
            int32_t  Int32;
            uint32_t Uint32;
        };
    };

    struct Param64 {
        Param64(double d) : Float64(d) {}
        Param64(int64_t i) : Int64(i) {}
        Param64(uint64_t u) : Uint64(u) {}

        void operator=(double f) { Float64 = f; }
        void operator=(int64_t i) { Int64 = i; }
        void operator=(uint64_t u) { Uint64 = u; }

        union
        {
            double   Float64;
            int64_t  Int64;
            uint64_t Uint64;
        };
    };

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

    class CommandContext
    {
        friend class ContextManager;
    private:
        CommandContext(D3D12_COMMAND_LIST_TYPE type);
        CommandContext(const CommandContext& other) = delete;
        CommandContext& operator=(const CommandContext& other) = delete;
        void Reset();

    public:
        CommandContext() = default;
        ~CommandContext();

        uint64_t Flush(bool waitForCompletion = false);
        uint64_t Finish(bool waitForCompletion = false);

        void Initialize();

        GraphicsContext& GetGraphicsContext();
        ComputeContext& GetComputeContext();
        uint64_t GetCompletedFenceValue();

        inline ID3D12GraphicsCommandList* GetCommandList() { return m_CommandList; }

        DynamicAllocation ReserveUploadMemory(size_t size) {
            return m_CpuLinearAllocator.Allocate(size);
        }

        static void InitializeTexture(GpuResource& destination, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA subData[]);
        static void InitializeBuffer(GpuResource& destination, const void* data, size_t sizeInBytes, size_t offset = 0);
        static void ReadbackTexture2D(GpuResource& readbackBuffer, Texture2D& texture);

        void CopyBuffer(GpuResource& source, GpuResource& destination);
        void CopyBufferRegion(GpuResource& destination, size_t offset, GpuResource& source, size_t srcOffset, size_t sizeInBytes);
        void CopySubresource(GpuResource& destination, uint32_t destinationIndex, GpuResource& source, uint32_t sourceIndex);
        
        void WriteBuffer(GpuResource& destination, size_t offset, const void* data, size_t sizeInBytes);

        void FlushResourceBarriers();

        void TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
        void InsertUAVBarrier(GpuResource& resource, bool flushImmediate);

        inline void TrackAllocation(HeapAllocationDescription& allocation, D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            
            switch (type)
            {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
                m_ResourceAllocations.push_back(allocation);
                allocation.Allocated = false;
                break;
            case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
                m_RTVAllocations.push_back(allocation);
                allocation.Allocated = false;
                break;
            default:
                HZ_CORE_ASSERT(false, "Unsupported heap type");
                break;
            }

        }
        inline void TrackAllocation(std::vector<HeapAllocationDescription>& allocations, D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            for (auto& allocation : allocations)
                TrackAllocation(allocation, type);
        }

        void TrackResource(GpuResource* resource);

        void BeginEvent(const char* name, uint64_t color = 0 /*PIX_COLOR_DEFAULT*/);
        void EndEvent();

        static CommandContext& Begin(const std::string name = "");

    protected:

        void SetName(const std::string& name) { m_Name = name; }
        void BindDescriptorHeaps();
        void ReturnAllocations();

    protected:
        D3D12_COMMAND_LIST_TYPE     m_Type;
        ID3D12GraphicsCommandList*  m_CommandList;
        ID3D12CommandAllocator*     m_CurrentAllocator;

        static constexpr uint8_t    MAX_RESOURCE_BARRIERS = 16;
        D3D12_RESOURCE_BARRIER      m_ResourceBarriers[MAX_RESOURCE_BARRIERS];
        uint64_t                    m_NumCachedBarriers;

        // 4 different types. But do I really need the sampler one?
        ID3D12DescriptorHeap*       m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        LinearAllocator m_CpuLinearAllocator;
        LinearAllocator m_GpuLinearAllocator;

        std::string m_Name;

        std::vector<HeapAllocationDescription> m_RTVAllocations;
        std::vector<HeapAllocationDescription> m_ResourceAllocations;
        std::vector<GpuResource*> m_TransientResources;
    };


    class GraphicsContext : public CommandContext {
    public:
        static GraphicsContext& Begin(const std::string name = "") {
            return CommandContext::Begin(name).GetGraphicsContext();
        }

        void SetDynamicContantBufferView(uint32_t rootIndex, size_t sizeInBytes, const void* data);
        void SetDynamicBufferAsTable(uint32_t rootIndex, size_t sizeInBytes, const void* data);
    };

    class ComputeContext : public CommandContext {
    public:

        static ComputeContext& Begin(const std::string& name = "", bool async = false);

        void SetShader(D3D12Shader& shader);

        void SetDynamicContantBufferView(uint32_t rootIndex, size_t sizeInBytes, const void* data);
    };

    class ContextManager {
    public:
        ContextManager() = default;

        CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE type);
        void FreeContext(CommandContext* context);
        void DestroyAll();
    private:
        std::vector<CommandContext*> m_ContextPool[4];
        std::queue<CommandContext*> m_AvailableContexts[4];
    };
}

