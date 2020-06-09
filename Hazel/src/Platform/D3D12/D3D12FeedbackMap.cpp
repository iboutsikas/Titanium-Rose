#include "hzpch.h"

#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/d3dx12.h"

namespace Hazel
{
	D3D12FeedbackMap::D3D12FeedbackMap(TComPtr<ID3D12Device2> device, uint32_t width, uint32_t height, uint32_t elementSize)
		: m_Width(width), m_Height(height), m_ElementSize(elementSize), m_IsMapped(false), m_Data(nullptr)
	{
		m_ActualSize = D3D12::CalculateConstantBufferSize(m_Width * m_Height * m_ElementSize);
		
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_ActualSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&m_Resource)
		));

		auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ActualSize);
		D3D12::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&readbackDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_Readback)
		));

		m_CurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	void D3D12FeedbackMap::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_Resource.Get(),
				from,
				to
			)
		);

		m_CurrentState = to;
	}

	void D3D12FeedbackMap::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES to)
	{
		if (to == m_CurrentState)
			return;

		this->Transition(cmdList, m_CurrentState, to);
	}

	void D3D12FeedbackMap::Map()
	{
		if (!m_IsMapped) {
			m_Readback->Map(0, nullptr, &m_Data);
			m_IsMapped = true;
		}
	}

	void D3D12FeedbackMap::Unmap()
	{
		if (m_IsMapped) {
			m_Readback->Unmap(0, nullptr);
			m_IsMapped = false;
		}
	}

	void D3D12FeedbackMap::Readback(ID3D12Device2* device, ID3D12GraphicsCommandList* cmdList)
	{
		this->Unmap();
		
		this->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmdList->CopyResource(m_Readback.Get(), m_Resource.Get());
		this->Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
}