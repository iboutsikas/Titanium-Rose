#include "hzpch.h"

#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/d3dx12.h"

namespace Hazel
{
	D3D12FeedbackMap::D3D12FeedbackMap(TComPtr<ID3D12Device2> device, uint32_t width, uint32_t height, uint32_t elementSize)
		: DeviceResource(width, height, 1, "", D3D12_RESOURCE_STATE_UNORDERED_ACCESS), 
		m_ElementSize(elementSize)
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

		m_ReadbackBuffer = CreateRef<ReadbackBuffer>(m_ActualSize);


	}

	void D3D12FeedbackMap::Update(ID3D12GraphicsCommandList* cmdList)
	{
		//m_ReadbackBuffer->Unmap();		
		this->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmdList->CopyResource(m_ReadbackBuffer->GetResource(), m_Resource.Get());
		this->Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
}