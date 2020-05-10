#pragma once


#include "Hazel/Renderer/Texture.h"

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/d3dx12.h"


namespace Hazel {
	class D3D12Context;

	class D3D12Texture2D : public Texture2D
	{
	public:
		D3D12Texture2D(uint32_t width, uint32_t height, uint32_t mipLevels = 1);
		D3D12Texture2D(const std::string& path);

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;
		
		void Transition(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
		void Transition(D3D12_RESOURCE_STATES to);
		void TransitionFeedback(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
		void TransitionFeedback(D3D12_RESOURCE_STATES to);

		void CreateFeedbackResource(UINT TileWidth, UINT TileHeight);

		inline ID3D12Resource* GetCommitedResource() const { return m_CommittedResource.Get(); }
		inline ID3D12Resource* GetFeedbackResource() const { return m_FeedbackResource.Get(); }
		inline ID3D12Resource* GetUploadResource() const { return m_UploadResource.Get(); }
		inline uint32_t GetMipLevels() const { return  m_MipLevels; }
		inline bool HasMips() const { return m_MipLevels > 1; }
		inline uint32_t  GetFeedbackSize() const { return m_FeedbackSize; }
		inline glm::ivec2 GetFeedbackDims() const { return m_FeedbackDims; }

		void DebugNameResource(std::wstring& name);

	private:
		std::string m_Path;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_MipLevels;
		uint32_t m_FeedbackSize;
		glm::ivec2 m_FeedbackDims;

		D3D12_RESOURCE_STATES m_CurrentState;
		D3D12_RESOURCE_STATES m_FeedbackState;
		D3D12Context* m_Context;
		TComPtr<ID3D12Resource> m_CommittedResource;
		TComPtr<ID3D12Resource> m_FeedbackResource;
		TComPtr<ID3D12Resource> m_UploadResource;
		std::vector<D3D12_SUBRESOURCE_DATA> m_SubData;
	};
}
