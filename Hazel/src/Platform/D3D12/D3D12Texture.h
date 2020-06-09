#pragma once


#include "Hazel/Renderer/Texture.h"

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12UploadBatch.h"


namespace Hazel {
	class D3D12Context;
	class D3D12TilePool;

	class D3D12Texture2D
	{
	public:
		struct TextureCreationOptions 
		{
			uint32_t Width;
			uint32_t Height;
			uint32_t MipLevels;
			std::wstring Path;
			std::wstring Name;
			D3D12_RESOURCE_FLAGS Flags;
		};

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		void SetData(D3D12ResourceUploadBatch& batch, void* data, uint32_t size);
		
		void Transition(D3D12ResourceUploadBatch& batch, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
		void Transition(D3D12ResourceUploadBatch& batch, D3D12_RESOURCE_STATES to);
		void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
		void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES to);

		inline Ref<D3D12FeedbackMap> GetFeedbackMap() const { return m_FeedbackMap; }
		inline void SetFeedbackMap(Ref<D3D12FeedbackMap> feedbackMap) { m_FeedbackMap = feedbackMap; }
		inline ID3D12Resource* GetResource() const { return m_Resource.Get(); }
		inline uint32_t GetMipLevels() const { return  m_MipLevels; }
		inline bool HasMips() const { return m_MipLevels > 1; }
		virtual bool IsVirtual() const = 0;


		static Ref<D3D12Texture2D>		CreateVirtualTexture(D3D12ResourceUploadBatch& batch, TextureCreationOptions& opts);
		static Ref<D3D12Texture2D>		CreateCommittedTexture(D3D12ResourceUploadBatch& batch, TextureCreationOptions& opts);
		static Ref<D3D12FeedbackMap>	CreateFeedbackMap(D3D12ResourceUploadBatch& batch, Ref<D3D12Texture2D> texture);
	protected:

		D3D12Texture2D(std::wstring id, uint32_t width, uint32_t height, uint32_t mips = 1);

		std::wstring m_Identifier;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_MipLevels;
		D3D12_RESOURCE_STATES m_CurrentState;

		TComPtr<ID3D12Resource> m_Resource;
		Ref<D3D12FeedbackMap> m_FeedbackMap;

	private:
		
	};

	class D3D12CommittedTexture2D : public D3D12Texture2D
	{
	public:
		D3D12CommittedTexture2D(std::wstring id, uint32_t width, uint32_t height, uint32_t mips = 1);
		D3D12CommittedTexture2D() = delete;
		virtual bool IsVirtual() const override { return false; }
	private:
		std::vector<D3D12_SUBRESOURCE_DATA> m_SubData;

		friend class D3D12Texture2D;
	};

	class D3D12VirtualTexture2D : public D3D12Texture2D
	{
	public:
		D3D12VirtualTexture2D(std::wstring id, uint32_t width, uint32_t height, uint32_t mips = 1);
		D3D12VirtualTexture2D() = delete;
		virtual bool IsVirtual() const override { return true; }

		struct TileAllocation
		{
			bool Mapped;
			uint32_t HeapOffset;
			D3D12_TILED_RESOURCE_COORDINATE ResourceCoordinate;
		};

		std::vector<std::vector<TileAllocation>> m_TileAllocations;

		friend class D3D12TilePool;
		friend class D3D12Texture2D;
	};
}
