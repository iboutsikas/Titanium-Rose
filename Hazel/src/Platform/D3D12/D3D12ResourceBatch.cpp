#include "hzpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"

namespace Hazel {
	D3D12ResourceBatch::D3D12ResourceBatch(TComPtr<ID3D12Device2> device)
		: m_Device(device),
		m_CommandList(nullptr),
		m_CommandAllocator(nullptr),
		m_Finalized(false)
	{
		
	}


	D3D12ResourceBatch::~D3D12ResourceBatch()
	{
		HZ_CORE_ASSERT(m_Finalized, "Resource batch was destructed without calling End()");
	}

	TComPtr<ID3D12GraphicsCommandList> D3D12ResourceBatch::Begin(D3D12_COMMAND_LIST_TYPE type)
	{
		switch (type)
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		case D3D12_COMMAND_LIST_TYPE_COPY:
			break;
		default:
			HZ_CORE_ASSERT(false, "Command List type not supported");
		}

		D3D12::ThrowIfFailed(m_Device->CreateCommandAllocator(
			type,
			IID_PPV_ARGS(&m_CommandAllocator)
		));

		D3D12::ThrowIfFailed(m_Device->CreateCommandList(
			1,
			type,
			m_CommandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&m_CommandList)
		));

		if (m_CommandAllocator == nullptr || m_CommandList == nullptr)
		{
			__debugbreak();
		}

#ifdef HZ_DEBUG
		m_CommandAllocator->SetName(L"ResourceUpload Command Allocator");
		m_CommandList->SetName(L"ResourceUpload Command List");
#endif
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
			m_CommandList.Get(), resource,
			tmpResource.Get(), 0, subresourceIndexStart,
			numSubresources, subResourceData
		);

		m_TrackedObjects.push_back(tmpResource);
	}

	void D3D12ResourceBatch::TrackResource(ID3D12Resource* resource)
	{
		m_TrackedObjects.emplace_back(resource);
	}

	std::future<void> D3D12ResourceBatch::End(ID3D12CommandQueue* commandQueue)
	{
		HZ_CORE_ASSERT(m_Finalized != true, "Resource batch has been finalized");

		D3D12::ThrowIfFailed(m_CommandList->Close());

		commandQueue->ExecuteCommandLists(1, CommandListCast(m_CommandList.GetAddressOf()));

		TComPtr<ID3D12Fence> fence;
		D3D12::ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

		HANDLE evt = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		HZ_CORE_ASSERT(evt, "Fence event was null");

		D3D12::ThrowIfFailed(commandQueue->Signal(fence.Get(), 1));
		D3D12::ThrowIfFailed(fence->SetEventOnCompletion(1, evt));

		auto batch = new Batch();
		batch->CommandList = m_CommandList;
		batch->Fence = fence;
		batch->GpuCompleteEvent = evt;
		std::swap(m_TrackedObjects, batch->TrackedObjects);

		std::future<void> ret = std::async(std::launch::async, [batch]() {
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

			delete batch;
		});

		m_Finalized = true;
		m_CommandList.Reset();
		m_CommandAllocator.Reset();
		return ret;
	}
}
