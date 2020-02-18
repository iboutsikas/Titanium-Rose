#pragma once

#include "Hazel/Renderer/Buffer.h"
#include "Platform/D3D12/ComPtr.h"
#include "d3d12.h"

namespace Hazel {
	// Forward Declarations
	class D3D12Context;

	class D3D12VertexBuffer : public VertexBuffer
	{
	public:
		D3D12VertexBuffer(float* vertices, uint32_t size);
		virtual ~D3D12VertexBuffer();

		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		inline TComPtr<ID3D12Resource> GetResource() { return m_CommittedResource; }
	private:
		BufferLayout m_Layout;
		D3D12Context* m_Context;
		TComPtr<ID3D12Resource> m_CommittedResource;
		TComPtr<ID3D12Resource> m_UploadResource;
	};

	class D3D12IndexBuffer : public IndexBuffer
	{
	public:
		D3D12IndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~D3D12IndexBuffer();

		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		virtual uint32_t GetCount() const { return m_Count; }
		inline TComPtr<ID3D12Resource> GetResource() { return m_CommittedResource; }
	private:
		uint32_t m_Count;
		D3D12Context* m_Context;
		TComPtr<ID3D12Resource> m_CommittedResource;
		TComPtr<ID3D12Resource> m_UploadResource;
	};

}
