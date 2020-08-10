#pragma once
#include "d3d12.h"
#include "glm/vec3.hpp"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/DeviceResource.h"
#include "Platform/D3D12/ReadbackBuffer.h"

namespace Hazel
{

	class D3D12FeedbackMap : public DeviceResource
	{
	public:
		D3D12FeedbackMap(TComPtr<ID3D12Device2> device, uint32_t width, uint32_t height, uint32_t elementSize);

		/// <summary>
		/// The elements of the vector are in order: Width, Height, Element Size
		/// </summary>
		/// <returns>The dimensions of the resource</returns>
		inline glm::ivec3 GetDimensions() const { return glm::ivec3(m_Width, m_Height, m_ElementSize); }
		inline const uint32_t GetElementSize() const { return m_ElementSize; }
		inline const uint32_t GetSize() const { return m_ActualSize; }

		void Update(ID3D12GraphicsCommandList* cmdList);

		uint32_t* GetData() { return m_ReadbackBuffer->Map<uint32_t*>(); }

		HeapAllocationDescription UAVAllocation;
	private:
		uint32_t m_ElementSize;
		uint32_t m_ActualSize;
		Ref<ReadbackBuffer> m_ReadbackBuffer;
	};

}
