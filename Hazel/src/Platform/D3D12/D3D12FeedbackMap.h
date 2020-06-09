#pragma once
#include "d3d12.h"
#include "glm/vec3.hpp"

#include "Platform/D3D12/ComPtr.h"

namespace Hazel
{
	class D3D12FeedbackMap
	{
	public:
		D3D12FeedbackMap(TComPtr<ID3D12Device2> device, uint32_t width, uint32_t height, uint32_t elementSize);

		void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
		void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES to);

		inline ID3D12Resource* GetResource() const { return m_Resource.Get(); }
		/// <summary>
		/// The elements of the vector are in order: Width, Height, Element Size
		/// </summary>
		/// <returns>The dimensions of the resource</returns>
		inline glm::ivec3 GetDimensions() const { return glm::ivec3(m_Width, m_Height, m_ElementSize); }
		inline const uint32_t GetWidth() const { return m_Width; }
		inline const uint32_t GetHeight() const { return m_Height; }
		inline const uint32_t GetElementSize() const { return m_ElementSize; }
		inline const uint32_t GetSize() const { return m_ActualSize; }
		inline const bool IsMapped() const { return m_IsMapped; }

		void Map();
		void Unmap();
		void Readback(ID3D12Device2* device, ID3D12GraphicsCommandList* cmdList);

		template<typename T,
			typename = std::is_pointer<T>()>
			const T GetData() {
			Map();
			return reinterpret_cast<T>(m_Data);
		}

	private:
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_ElementSize;
		uint32_t m_ActualSize;
		bool m_IsMapped;
		void* m_Data;
		D3D12_RESOURCE_STATES m_CurrentState;
		TComPtr<ID3D12Resource> m_Resource;
		TComPtr<ID3D12Resource> m_Readback;
	};

}
