#pragma once

#include "Hazel/Core/Application.h"
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
		inline D3D12_VERTEX_BUFFER_VIEW GetView() const { return m_View; }
	private:
		BufferLayout m_Layout;
		D3D12Context* m_Context;
		TComPtr<ID3D12Resource> m_CommittedResource;
		TComPtr<ID3D12Resource> m_UploadResource;
		D3D12_VERTEX_BUFFER_VIEW m_View;
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
		inline D3D12_INDEX_BUFFER_VIEW GetView() const { return m_View; }
	private:
		uint32_t m_Count;
		D3D12Context* m_Context;
		TComPtr<ID3D12Resource> m_CommittedResource;
		TComPtr<ID3D12Resource> m_UploadResource;
		D3D12_INDEX_BUFFER_VIEW m_View;
	};
	
	template<typename T>
	class D3D12UploadBuffer
	{
	public:
		D3D12UploadBuffer(uint32_t elementCount, bool isConstantBuffer)
			: m_IsConstantBuffer(isConstantBuffer)
		{
			m_ElementByteSize = sizeof(T);

			if (m_IsConstantBuffer) {
				m_ElementByteSize = D3D12::CalculateConstantBufferSize(m_ElementByteSize);
			}

			m_Context = static_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());

			auto device = m_Context->DeviceResources->Device;

			D3D12::ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_UploadBuffer)
			));

			D3D12::ThrowIfFailed(m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
		}

		D3D12UploadBuffer(const D3D12UploadBuffer& rhs) = delete;
		D3D12UploadBuffer& operator=(const D3D12UploadBuffer& rhs) = delete;
		
		~D3D12UploadBuffer()
		{
			if (m_UploadBuffer != nullptr)
				m_UploadBuffer->Unmap(0, nullptr);

			m_MappedData = nullptr;
		}

		inline ID3D12Resource* Resource() const { return m_UploadBuffer.Get(); }

		inline size_t CalculateOffset(uint32_t numElements) const { return numElements * m_ElementByteSize; }

		void CopyData(int elementIndex, const T& data)
		{
			auto size = sizeof(T);
			auto offset = (elementIndex * m_ElementByteSize);
			memcpy(m_MappedData + offset, &data, size);
		}

	private:
		TComPtr<ID3D12Resource> m_UploadBuffer;
		uint8_t* m_MappedData = nullptr;
		D3D12Context* m_Context;

		uint32_t m_ElementByteSize = 0;
		bool m_IsConstantBuffer = false;
	};

}
