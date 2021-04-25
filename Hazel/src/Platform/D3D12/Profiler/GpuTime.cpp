#include <hzpch.h>
#include <d3d12.h>

#include "Platform/D3D12/Profiler/GpuTime.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/CommandQueue.h"

namespace Hazel { namespace GpuTime {

    
    D3D12DeviceResources* r = nullptr; 
    D3D12Context* c = nullptr;

    uint64_t* s_TimestampBuffer;
    uint64_t s_FenceValue = 0;
    uint32_t s_MaxTimers = 0;
    uint32_t s_NumTimers = 1;
    uint64_t s_ValidTimeStart = 0;
    uint64_t s_ValidTimeEnd = 0;
    double s_GpuTickDelta = 0;

    Scope<ReadbackBuffer> s_ReadbackBuffer;
    ID3D12QueryHeap* s_QueryHeap = nullptr;

    void Initialize(uint32_t maxTimers)
    {
        r = D3D12Renderer::Context->DeviceResources.get();
        c = D3D12Renderer::Context;

        uint64_t gpuFrequency;
        D3D12::ThrowIfFailed(D3D12Renderer::CommandQueueManager.GetCommandQueue()->GetTimestampFrequency(&gpuFrequency));
        s_GpuTickDelta = 1.0 / (double)gpuFrequency;
        
        s_ReadbackBuffer = CreateScope<ReadbackBuffer>(maxTimers * 2 * sizeof(uint64_t));
        s_ReadbackBuffer->SetName("Timestamp Readback Buffer");

        D3D12_QUERY_HEAP_DESC QueryHeapDesc;
        QueryHeapDesc.Count = maxTimers * 2;
        QueryHeapDesc.NodeMask = 1;
        QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        D3D12::ThrowIfFailed(r->Device->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&s_QueryHeap)));
        s_QueryHeap->SetName(L"Timestamp QueryHeap");

        s_MaxTimers = maxTimers;
    }

    void Shutdown()
    {
        if (s_ReadbackBuffer)
            s_ReadbackBuffer.release();

        if (s_QueryHeap)
            s_QueryHeap->Release();
    }

    uint32_t NewTimer(void)
    {
        return s_NumTimers++;
    }

    void StartTimer(CommandContext& context, uint32_t index)
    {
        context.GetCommandList()->EndQuery(s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index * 2);
    }

    void StopTimer(CommandContext& context, uint32_t index)
    {
        context.GetCommandList()->EndQuery(s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, (index * 2) + 1);
    }

    void BeginReadBack(void)
    {
        D3D12Renderer::CommandQueueManager.WaitForFence(s_FenceValue);

        s_TimestampBuffer = s_ReadbackBuffer->Map<uint64_t*>(0, s_NumTimers * 2 * sizeof(uint64_t));

        s_ValidTimeStart = s_TimestampBuffer[0];
        s_ValidTimeEnd   = s_TimestampBuffer[1];

        if (s_ValidTimeEnd < s_ValidTimeStart) {
            s_ValidTimeStart = 0;
            s_ValidTimeEnd = 0;
        }
    }
    
    void EndReadBack(void)
    {
        s_ReadbackBuffer->Unmap();
        s_TimestampBuffer = nullptr;

        CommandContext& context = CommandContext::Begin();
        context.GetCommandList()->EndQuery(s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
        context.GetCommandList()->ResolveQueryData(s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, s_NumTimers * 2, s_ReadbackBuffer->GetResource(), 0);
        context.GetCommandList()->EndQuery(s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
        s_FenceValue = context.Finish();
    }


    float GetTime(uint32_t index)
    {
        HZ_CORE_ASSERT(s_TimestampBuffer != nullptr, "Timestamp buffer is not mapped");
        HZ_CORE_ASSERT(index < s_NumTimers, "Timer index is out of bounds");

        auto t1 = s_TimestampBuffer[index * 2];
        auto t2 = s_TimestampBuffer[index * 2 + 1];

        if (t1 < s_ValidTimeStart || t2 > s_ValidTimeEnd || t2 <= t1)
            return 0.0f;
        // Need to divide by gpu frequency, according to documentation
        return static_cast<float>((t2 - t1) * s_GpuTickDelta);
    }
}}