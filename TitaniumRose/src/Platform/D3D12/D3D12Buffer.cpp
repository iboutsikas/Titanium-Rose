#include "trpch.h"
#include "TitaniumRose/Core/Application.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Renderer.h"

#include <d3d12.h>
#include "Platform/D3D12/d3dx12.h"

namespace Roses {
	
	D3D12VertexBuffer::D3D12VertexBuffer(CommandContext& context, float* vertices, uint32_t size)
	{
		

		D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_Resource.GetAddressOf())
		));
		BypassAndSetState(D3D12_RESOURCE_STATE_COPY_DEST);

		if (vertices) {
			context.WriteBuffer(*this, 0, vertices, size);

			m_View.BufferLocation = m_Resource->GetGPUVirtualAddress();
			m_View.SizeInBytes = size;
		}
	}
	
	D3D12VertexBuffer::~D3D12VertexBuffer()
	{
	}

	D3D12IndexBuffer::D3D12IndexBuffer(CommandContext& context, uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		const uint32_t size = sizeof(uint32_t) * count;

		D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_Resource.GetAddressOf())
		));
		BypassAndSetState(D3D12_RESOURCE_STATE_COPY_DEST);

		if (indices) {
			context.WriteBuffer(*this, 0, indices, size);

			m_View.BufferLocation = m_Resource->GetGPUVirtualAddress();
			m_View.Format = DXGI_FORMAT_R32_UINT;
			m_View.SizeInBytes = size;
		}
	}

	D3D12IndexBuffer::~D3D12IndexBuffer()
	{
	}
}
