#pragma once

#include "Hazel/Core/Image.h"
//#include "Hazel/Core/Math/Hash.h"

#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/GpuBuffer.h"

#include "d3d12.h"
#include "glm/vec3.hpp"

namespace Hazel {
    class TextureManager;
    class D3D12FeedbackMap;
    
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

    class Texture : public GpuBuffer {
        friend class TextureManager;
    public:

        virtual ~Texture() = default;

        bool IsCube() const { return m_IsCube; }

        inline uint32_t GetMipLevels() const { return  m_MipLevels; }
        inline bool HasMips() const { return m_MipLevels > 1; }
        inline uint64_t GetGPUSizeInBytes() const { return m_ResourceAllocationInfo.SizeInBytes; }

    protected:
        uint32_t m_MipLevels;
        bool m_IsCube;

        Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t mips,
            bool isCube, D3D12_RESOURCE_STATES initialState, std::string id);

        D3D12_RESOURCE_ALLOCATION_INFO m_ResourceAllocationInfo;

        void UpdateFromDescription();
    };

    class Texture2D : public Texture
    {

    public:
        struct MipLevelsUsed {
            uint32_t FinestMip;
            uint32_t CoarsestMip;
        };

        virtual ~Texture2D() = default;

        inline D3D12FeedbackMap* GetFeedbackMap() const { return m_FeedbackMap; }
        inline void SetFeedbackMap(D3D12FeedbackMap* feedbackMap) { m_FeedbackMap = feedbackMap; }

        virtual bool IsVirtual() const { return false; }
        virtual MipLevelsUsed ExtractMipsUsed() { return { 0, m_MipLevels - 1 }; }

        // If the texture is virtual you need to call ExtractMipsUsed at least
        // once before calling this. This will return cached data which might be 
        // stale
        virtual MipLevelsUsed GetMipsUsed() { return { 0, m_MipLevels - 1 }; }



        static Ref<Texture2D>		    CreateVirtualTexture(struct TextureCreationOptions& opts);
        static Ref<Texture2D>		    CreateCommittedTexture(struct TextureCreationOptions& opts);
        static D3D12FeedbackMap*        CreateFeedbackMap(Texture2D& texture);

    protected:

        static Ref<Texture2D> LoadFromDDS(TextureCreationOptions& opts);
        static Ref<Texture2D> LoadFromImage(Ref<Image>& image, TextureCreationOptions& opts);

        Texture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips = 1);

        D3D12FeedbackMap* m_FeedbackMap;
    };

    class CommittedTexture2D : public Texture2D
    {
    public:
        CommittedTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips = 1);
        CommittedTexture2D() = delete;
        virtual bool IsVirtual() const override { return false; }
    private:
        std::vector<D3D12_SUBRESOURCE_DATA> m_SubData;

        friend class Texture2D;
    };

    class VirtualTexture2D : public Texture2D
    {
    public:
        VirtualTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips = 1);
        VirtualTexture2D() = delete;
        virtual ~VirtualTexture2D();

        virtual bool IsVirtual() const override { return true; }

        virtual MipLevelsUsed ExtractMipsUsed() override;
        virtual MipLevelsUsed GetMipsUsed() override;

        glm::ivec3 GetTileDimensions(uint32_t subresource = 0) const;
        uint64_t GetTileUsage();

        uint32_t								    m_NumTiles;
        D3D12_TILE_SHAPE						    m_TileShape;
        D3D12_PACKED_MIP_INFO					    m_MipInfo;
        std::vector<D3D12_SUBRESOURCE_TILING>	    m_Tilings;
    private:
        MipLevelsUsed m_CachedMipLevels;
        friend class D3D12TilePool;
        friend class Texture2D;
    };

#pragma region Cube Texture
    class D3D12TextureCube : public Texture
    {
    public:
        static Ref<D3D12TextureCube> Initialize(TextureCreationOptions& opts);
        D3D12TextureCube(uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, std::string id);
    };
#pragma endregion
}
