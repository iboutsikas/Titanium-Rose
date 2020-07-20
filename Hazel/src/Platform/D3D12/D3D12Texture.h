#pragma once


#include "Hazel/Renderer/Texture.h"

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12FeedbackMap.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"


namespace Hazel {
    class D3D12Context;
    class D3D12TilePool;

    class D3D12Texture
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

            TextureCreationOptions() :
                Width(1), Height(1), Depth(1), MipLevels(1),
                Path(""), Name(""), 
                Flags(D3D12_RESOURCE_FLAG_NONE),
                Format(DXGI_FORMAT_R8G8B8A8_UNORM)
            {}
        };

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint32_t GetDepth() const { return m_Depth; }
        bool IsCube() const { return m_IsCube; }

        void Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
        void Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES to);
        void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
        void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES to);

        inline ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        inline uint32_t GetMipLevels() const { return  m_MipLevels; }
        inline bool HasMips() const { return m_MipLevels > 1; }
        inline std::string GetIdentifier() const { return m_Identifier; }
        inline DXGI_FORMAT GetFormat() const { return m_Resource->GetDesc().Format; }
        HeapAllocationDescription DescriptorAllocation;

    protected:
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Depth;
        uint32_t m_MipLevels;
        bool m_IsCube;
        
        D3D12_RESOURCE_STATES m_CurrentState;

        std::string m_Identifier;

        TComPtr<ID3D12Resource> m_Resource;

        D3D12Texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t mips,
            bool isCube, D3D12_RESOURCE_STATES initialState, std::string id);

    };

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

        struct TileAllocation
        {
            bool Mapped;
            uint32_t HeapOffset;
            D3D12_TILED_RESOURCE_COORDINATE ResourceCoordinate;
        };

        std::vector<std::vector<TileAllocation>>    m_TileAllocations;
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
