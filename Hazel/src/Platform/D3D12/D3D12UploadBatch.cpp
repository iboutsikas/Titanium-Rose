#include "hzpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12UploadBatch.h"

namespace Hazel {
	D3D12ResourceUploadBatch::D3D12ResourceUploadBatch(TComPtr<ID3D12Device2> device)
		: m_Device(device),
		m_CommandList(nullptr),
		m_CommandAllocator(nullptr),
		m_Finalized(false)
	{
		
	}

	D3D12ResourceUploadBatch::~D3D12ResourceUploadBatch()
	{
		HZ_CORE_ASSERT(m_Finalized, "Resource batch was destructed without calling End()");
	}

	TComPtr<ID3D12GraphicsCommandList> D3D12ResourceUploadBatch::Begin(D3D12_COMMAND_LIST_TYPE type)
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

#ifdef HZ_DEBUG
		m_CommandAllocator->SetName(L"ResourceUpload Command Allocator");
		m_CommandList->SetName(L"ResourceUpload Command List");
#endif
		return m_CommandList;
	}

	std::future<void> D3D12ResourceUploadBatch::End(ID3D12CommandQueue* commandQueue)
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

		return ret;
	}
}
