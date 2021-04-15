#include "hzpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/Profiler/Profiler.h"

namespace Hazel {
	D3D12ResourceBatch::D3D12ResourceBatch(TComPtr<ID3D12Device2> device, Ref<D3D12CommandList> commandList, bool async) :
		m_Device(device),
		m_CommandList(commandList),
		m_Finalized(false),
		m_IsAsync(async)
	{
		
	}

	D3D12ResourceBatch::~D3D12ResourceBatch()
	{
		HZ_CORE_ASSERT(m_Finalized, "Resource batch was destructed without calling End()");
	}

	Ref<D3D12CommandList> D3D12ResourceBatch::Begin(TComPtr<ID3D12CommandAllocator> commandAllocator)
	{
		if (m_CommandList->IsClosed())
			m_CommandList->Reset(commandAllocator);
		return m_CommandList;
	}

	void D3D12ResourceBatch::Upload(ID3D12Resource* resource, uint32_t subresourceIndexStart, const D3D12_SUBRESOURCE_DATA* subResourceData, uint32_t numSubresources)
	{
		HZ_CORE_ASSERT(m_Finalized != true, "Batch has been finalized");

		uint64_t uploadSize = GetRequiredIntermediateSize(
			resource,
			subresourceIndexStart,
			numSubresources
		);

		TComPtr<ID3D12Resource> tmpResource = nullptr;
		D3D12::ThrowIfFailed(m_Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(tmpResource.GetAddressOf())
		));

		UpdateSubresources(
			m_CommandList->GetRawPtr(), resource,
			tmpResource.Get(), 0, subresourceIndexStart,
			numSubresources, subResourceData
		);

		m_TrackedObjects.push_back(tmpResource);
	}

	void D3D12ResourceBatch::TrackResource(TComPtr<ID3D12Resource> resource)
	{
		HZ_CORE_ASSERT(m_IsAsync, "Calling a track method on a non-async batch is a bad idea");

		if (m_IsAsync)
			m_TrackedObjects.emplace_back(resource);
	}

	void D3D12ResourceBatch::TrackImage(Ref<Image> image)
	{
		HZ_CORE_ASSERT(m_IsAsync, "Calling a track method on a non-async batch is a bad idea");
		if (m_IsAsync)
			m_TrackedImages.push_back(image);
	}

	void D3D12ResourceBatch::TrackBlock(CPUProfileBlock& block)
	{
		m_CPUProfilingBlocks.push(std::move(block));
	}

    void D3D12ResourceBatch::TrackBlock(GPUProfileBlock& block)
    {
		m_GPUProfilingBlocks.push(std::move(block));
    }

	void D3D12ResourceBatch::End(TComPtr<ID3D12CommandQueue> commandQueue)
	{
		HZ_CORE_ASSERT(m_Finalized == false, "Resource batch has been finalized");
		HZ_CORE_ASSERT(m_IsAsync == false, "Should not be calling a sync method on an async batch");

        while (!m_CPUProfilingBlocks.empty())
            m_CPUProfilingBlocks.pop();
        while (!m_GPUProfilingBlocks.empty())
            m_GPUProfilingBlocks.pop();

		m_CommandList->Execute(commandQueue);
		m_Finalized = true;
		return;
	}

	void D3D12ResourceBatch::EndAndWait(TComPtr<ID3D12CommandQueue> commandQueue)
	{
        HZ_CORE_ASSERT(m_Finalized == false, "Resource batch has been finalized");
        HZ_CORE_ASSERT(m_IsAsync == false, "Should not be calling a sync method on an async batch");

        while (!m_CPUProfilingBlocks.empty())
            m_CPUProfilingBlocks.pop();
        while (!m_GPUProfilingBlocks.empty())
            m_GPUProfilingBlocks.pop();

        m_CommandList->ExecuteAndWait(commandQueue);
        m_Finalized = true;
        return;
	}

    std::future<void> D3D12ResourceBatch::EndAsync(TComPtr<ID3D12CommandQueue> commandQueue)
    {
        HZ_CORE_ASSERT(m_IsAsync, "Should not be calling an async method in a non-async batch");

        while (!m_CPUProfilingBlocks.empty())
            m_CPUProfilingBlocks.pop();
        while (!m_GPUProfilingBlocks.empty())
            m_GPUProfilingBlocks.pop();

		m_CommandList->Execute(commandQueue);

        TComPtr<ID3D12Fence> fence;
        D3D12::ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        HANDLE evt = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        HZ_CORE_ASSERT(evt, "Fence event was null");

        D3D12::ThrowIfFailed(commandQueue->Signal(fence.Get(), 1));
        D3D12::ThrowIfFailed(fence->SetEventOnCompletion(1, evt));

        auto batch = new Batch();
		batch->GpuCompleteEvent = m_CommandList->AddFenceEvent(commandQueue);
        std::swap(m_TrackedObjects, batch->TrackedObjects);
        std::swap(m_TrackedImages, batch->TrackedImages);

        m_Finalized = true;

        return std::async(std::launch::async, [batch]() {

            auto wr = WaitForSingleObject(batch->GpuCompleteEvent, INFINITE);

            if (wr != WAIT_OBJECT_0)
            {
                if (wr == WAIT_FAILED)
                {
                    D3D12::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
                }
                else
                {
                    HZ_CORE_ASSERT(false, "WTH happened here ?");
                }
            }
            batch->TrackedObjects.clear();
            batch->TrackedImages.clear();
            CloseHandle(batch->GpuCompleteEvent);
            delete batch;
        });
    }
}
