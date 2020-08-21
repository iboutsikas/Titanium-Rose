#pragma once

#include "Hazel/Core/Image.h"
#include "Hazel/Renderer/Texture.h"

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/DeviceResource.h"

const float clrclr[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

namespace Hazel {
    class D3D12Context;
    class D3D12TilePool;

#pragma region Texture
    class D3D12Texture : public DeviceResource
    {
    public:
        struct TextureCreationOptions
        {
            uint32_t Width;
            uint32_t Height;
            uint32_t Depth;
            uint32_t MipLevels;
            std::string Path;
            std::string Name;
            D3D12_RESOURCE_FLAGS Flags;
            DXGI_FORMAT Format;
            bool IsDepthStencil;

            TextureCreationOptions() :
                Width(1), Height(1), Depth(1), MipLevels(1),
                Path(""), Name(""),
                Flags(D3D12_RESOURCE_FLAG_NONE),
                Format(DXGI_FORMAT_R8G8B8A8_UNORM),
                IsDepthStencil(false)
            {}
        };

        bool IsCube() const { return m_IsCube; }

        inline uint32_t GetMipLevels() const { return  m_MipLevels; }
        inline bool HasMips() const { return m_MipLevels > 1; }

        HeapAllocationDescription SRVAllocation;
        HeapAllocationDescription UAVAllocation;
        HeapAllocationDescription RTVAllocation;

    protected:
        uint32_t m_MipLevels;
        bool m_IsCube;

        D3D12Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t mips,
            bool isCube, D3D12_RESOURCE_STATES initialState, std::string id);

    };
#pragma endregion


#pragma region Texture2D
    class D3D12Texture2D: public D3D12Texture
    {
    public:
        struct MipLevels {
            uint32_t FinestMip;
            uint32_t CoarsestMip;
        };

        void SetData(D3D12ResourceBatch& batch, void* data, uint32_t size);

        inline Ref<D3D12FeedbackMap> GetFeedbackMap() const { return m_FeedbackMap; }
        inline void SetFeedbackMap(Ref<D3D12FeedbackMap> feedbackMap) { m_FeedbackMap = feedbackMap; }

        virtual bool IsVirtual() const = 0;
        virtual MipLevels ExtractMipsUsed() { return { 0, m_MipLevels - 1 }; }
        // If the texture is virtual you need to call ExtractMipsUsed at least
        // once before calling this. This will return cached data which might be 
        // stale
        virtual MipLevels GetMipsUsed() { return { 0, m_MipLevels - 1 }; }

        static Ref<D3D12Texture2D>		CreateVirtualTexture(D3D12ResourceBatch& batch, TextureCreationOptions& opts);
        static Ref<D3D12Texture2D>		CreateCommittedTexture(D3D12ResourceBatch& batch, TextureCreationOptions& opts);
        static Ref<D3D12FeedbackMap>	CreateFeedbackMap(D3D12ResourceBatch& batch, Ref<D3D12Texture2D> texture);

    protected:

        static Ref<D3D12Texture2D> LoadFromDDS(D3D12ResourceBatch& batch, TextureCreationOptions& opts);

        static Ref<D3D12Texture2D> LoadFromImage(Ref<Image>& image, D3D12ResourceBatch& batch, TextureCreationOptions& opts);

        D3D12Texture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips = 1);

        Ref<D3D12FeedbackMap> m_FeedbackMap;
    };

    class D3D12CommittedTexture2D : public D3D12Texture2D
    {
    public:
        D3D12CommittedTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips = 1);
        D3D12CommittedTexture2D() = delete;
        virtual bool IsVirtual() const override { return false; }
    private:
        std::vector<D3D12_SUBRESOURCE_DATA> m_SubData;

        friend class D3D12Texture2D;
    };

    class D3D12VirtualTexture2D : public D3D12Texture2D
    {
    public:
        D3D12VirtualTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips = 1);
        D3D12VirtualTexture2D() = delete;

        virtual bool IsVirtual() const override { return true; }

        virtual MipLevels ExtractMipsUsed() override;
        virtual MipLevels GetMipsUsed() override;

        glm::ivec3 GetTileDimensions(uint32_t subresource = 0) const;

        uint32_t								    m_NumTiles;
        D3D12_TILE_SHAPE						    m_TileShape;
        D3D12_PACKED_MIP_INFO					    m_MipInfo;
        std::vector<D3D12_SUBRESOURCE_TILING>	    m_Tilings;
    private:
        MipLevels m_CachedMipLevels;
        friend class D3D12TilePool;
        friend class D3D12Texture2D;
    };

#pragma endregion

#pragma region Cube Texture
    class D3D12TextureCube : public D3D12Texture
    {
    public:
        static Ref<D3D12TextureCube> Create(D3D12ResourceBatch& batch, TextureCreationOptions& opts);
        D3D12TextureCube(uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, std::string id);

    protected:
    };
#pragma endregion
}
