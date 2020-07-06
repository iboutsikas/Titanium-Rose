#pragma once

#include "d3d12.h"

#include "Hazel/Renderer/Buffer.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"


namespace Hazel {
	class D3D12ResourceBatch;

	class D3D12VertexBuffer : public VertexBuffer
	{
	public:
		D3D12VertexBuffer(D3D12ResourceBatch& batch, float* vertices, uint32_t size);
		virtual ~D3D12VertexBuffer();

		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		inline TComPtr<ID3D12Resource> GetResource() { return m_CommittedResource; }
		inline D3D12_VERTEX_BUFFER_VIEW GetView() const { return m_View; }
	private:
		BufferLayout m_Layout;
		TComPtr<ID3D12Resource> m_CommittedResource;
		D3D12_VERTEX_BUFFER_VIEW m_View;
	};

	class D3D12IndexBuffer : public IndexBuffer
	{
	public:
		D3D12IndexBuffer(D3D12ResourceBatch& batch, uint32_t* indices, uint32_t count);
		virtual ~D3D12IndexBuffer();

		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		virtual uint32_t GetCount() const { return m_Count; }
		inline TComPtr<ID3D12Resource> GetResource() { return m_CommittedResource; }
		inline D3D12_INDEX_BUFFER_VIEW GetView() const { return m_View; }
	private:
		uint32_t m_Count;
		TComPtr<ID3D12Resource> m_CommittedResource;
		D3D12_INDEX_BUFFER_VIEW m_View;
	};
#if 1
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

			D3D12::ThrowIfFailed(D3D12Renderer::Context->DeviceResources->Device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_UploadBuffer)
			));

			D3D12::ThrowIfFailed(m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
		}

		D3D12UploadBuffer(D3D12ResourceBatch& batch, uint32_t elementCount, bool isConstantBuffer)
			: m_IsConstantBuffer(isConstantBuffer)
		{
			m_ElementByteSize = sizeof(T);

			if (m_IsConstantBuffer) {
				m_ElementByteSize = D3D12::CalculateConstantBufferSize(m_ElementByteSize);
			}

			D3D12::ThrowIfFailed(batch.GetDevice()->CreateCommittedResource(
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

		inline T* ElementAt(uint32_t index) 
		{
			return (T*)(m_MappedData + (index * m_ElementByteSize));
		}

	private:
		TComPtr<ID3D12Resource> m_UploadBuffer;
		uint8_t* m_MappedData = nullptr;

		uint32_t m_ElementByteSize = 0;
		bool m_IsConstantBuffer = false;
	};
#endif
}

