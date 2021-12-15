#include "trpch.h"

#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/CommandContext.h"

namespace Roses
{
	D3D12FeedbackMap::D3D12FeedbackMap(ID3D12Device2* device, uint32_t width, uint32_t height, size_t elementSize)
		: GpuBuffer(width, height, 1, "", D3D12_RESOURCE_STATE_UNORDERED_ACCESS), 
		m_ElementSize(elementSize), m_ReadbackBuffer(nullptr)
	{
		m_ActualSize = D3D12::AlignUp(m_Width * m_Height * m_ElementSize);
		
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_ActualSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&m_Resource)
		));

		m_ReadbackBuffer = new ReadbackBuffer(m_ActualSize);


	}

	D3D12FeedbackMap::~D3D12FeedbackMap()
	{
		delete m_ReadbackBuffer;
	}

	void D3D12FeedbackMap::Update(CommandContext& context)
	{		
		context.CopyBuffer(*this, *m_ReadbackBuffer);
	}
}