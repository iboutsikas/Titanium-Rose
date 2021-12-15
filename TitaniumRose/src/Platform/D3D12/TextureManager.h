#pragma once
#include "Platform/D3D12/D3D12Texture.h"

#include <d3d12.h>
#include <tuple>

namespace Roses {
    class Texture2D;

    class TextureManager {
    public:

        TextureManager() = default;
        ~TextureManager() {
            for (int i = 0; i < m_AllTextures.size(); i++)
            {
                delete m_AllTextures[i];
            }
            m_AllTextures.clear();
        }

        static Texture2D* RequestTexture(Texture2D* referenceTexture, uint64_t fenceValue);
        static Texture2D* RequestTemporaryRenderTarget(uint32_t width, uint32_t height, DXGI_FORMAT format, uint64_t fenceValue);
        static void DiscardTexture(Texture2D* texture, uint64_t fenceValue);


    private:

        struct TextureCacheIndex {
            uint32_t Width;
            uint32_t Height;
            uint32_t Depth;
            DXGI_FORMAT Format;

            bool operator==(TextureCacheIndex const& other) const {
                return std::tie(Width, Height, Depth, Format) == std::tie(other.Width, other.Height, other.Depth, other.Format);
            }
        };

        struct TextureCacheHash {
            size_t operator() (TextureCacheIndex const& index) const;
        };

        struct ReadyTexture {
            uint64_t FenceValue;
            Texture2D* TexturePtr;
        };

        using TextureQueue = std::queue<ReadyTexture>;

        Texture2D* RequestTexture(TextureCacheIndex& index, uint64_t fenceValue);
        void DiscardTexture(TextureCacheIndex& index, Texture2D* texture, uint64_t fenceValue);

        std::unordered_map<TextureCacheIndex, TextureQueue, TextureCacheHash> m_AvailableTextures;
        std::vector<Texture2D*> m_AllTextures;

        static TextureManager s_Instance;
    };
}
