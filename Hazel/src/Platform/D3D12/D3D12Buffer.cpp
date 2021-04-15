#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"

#include <d3d12.h>
#include "Platform/D3D12/d3dx12.h"

namespace Hazel {
	
	D3D12VertexBuffer::D3D12VertexBuffer(D3D12ResourceBatch& batch, float* vertices, uint32_t size)
	{
		auto device = batch.GetDevice();

		D3D12::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_CommittedResource.GetAddressOf())
		));

		if (vertices) {
			
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = vertices;
			subresourceData.RowPitch = size;
			subresourceData.SlicePitch = subresourceData.RowPitch;

			batch.Upload(m_CommittedResource.Get(), 0, &subresourceData, 1);

			m_View.BufferLocation = m_CommittedResource->GetGPUVirtualAddress();
			m_View.SizeInBytes = size;
		}
	}
	
	D3D12VertexBuffer::~D3D12VertexBuffer()
	{
	}

	D3D12IndexBuffer::D3D12IndexBuffer(D3D12ResourceBatch& batch, uint32_t* indices, uint32_t count)
		:m_Count(count)
	{
		auto device = batch.GetDevice();
		auto size = sizeof(uint32_t) * count;

		D3D12::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_CommittedResource.GetAddressOf())
		));

		if (indices) {

			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = indices;
			subresourceData.RowPitch = size;
			subresourceData.SlicePitch = subresourceData.RowPitch;

			batch.Upload(m_CommittedResource.Get(), 0, &subresourceData, 1);

			m_View.BufferLocation = m_CommittedResource->GetGPUVirtualAddress();
			m_View.Format = DXGI_FORMAT_R32_UINT;
			m_View.SizeInBytes = size;
		}
	}

	D3D12IndexBuffer::~D3D12IndexBuffer()
	{
	}
}
