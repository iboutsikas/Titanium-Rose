#include "trpch.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/TextureManager.h"

#include "TitaniumRose/Core/Math/Hash.h"
namespace Roses {
    TextureManager TextureManager::s_Instance;


    Texture2D* TextureManager::RequestTexture(Texture2D* referenceTexture, uint64_t fenceValue)
    {
        TextureCacheIndex index = {};
        index.Width = referenceTexture->GetWidth();
        index.Height = referenceTexture->GetHeight();
        index.Depth = referenceTexture->GetDepth();
        index.Format = referenceTexture->GetFormat();

        return s_Instance.RequestTexture(index, fenceValue);
    }

    Texture2D* TextureManager::RequestTemporaryRenderTarget(uint32_t width, uint32_t height, DXGI_FORMAT format, uint64_t fenceValue)
    {
        TextureCacheIndex index = {};
        index.Width = width;
        index.Height = height;
        index.Depth = 1;
        index.Format = format;
        return s_Instance.RequestTexture(index, fenceValue);
        return nullptr;
    }

    void TextureManager::DiscardTexture(Texture2D* texture, uint64_t fenceValue)
    {
        TextureCacheIndex index = {};
        index.Width = texture->GetWidth();
        index.Height = texture->GetHeight();
        index.Depth = texture->GetDepth();
        index.Format = texture->GetFormat();
        s_Instance.DiscardTexture(index, texture, fenceValue);
    }

    Texture2D* TextureManager::RequestTexture(TextureCacheIndex& index, uint64_t fenceValue)
    {
        Texture2D* texture = nullptr;
        auto search = m_AvailableTextures.find(index);

        if (search == m_AvailableTextures.end()) {
            TextureQueue temporaryQueue;
            m_AvailableTextures[index] = temporaryQueue;
        }

        TextureQueue& readyQueue = m_AvailableTextures[index];

        if (!readyQueue.empty()) {
            ReadyTexture& readyTexture = readyQueue.front();

            if (readyTexture.FenceValue <= fenceValue) {
                texture = readyTexture.TexturePtr;
                readyQueue.pop();
                HZ_CORE_ASSERT(!texture->RTVAllocation.Allocated, "");
                HZ_CORE_ASSERT(!texture->SRVAllocation.Allocated, "");
            }

        }

        if (texture == nullptr) {
            texture = new CommittedTexture2D("Temporary Texture", index.Width, index.Height, 1);

            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.MipLevels = 1;
            textureDesc.Format = index.Format;
            textureDesc.Width = index.Width;
            textureDesc.Height = index.Height;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &textureDesc,
                texture->m_CurrentState,
                nullptr,
                IID_PPV_ARGS(texture->m_Resource.GetAddressOf())
            ));

            texture->SetName("Temporary Texture");
            texture->UpdateFromDescription();

            m_AllTextures.push_back(texture);
        }

        return texture;

    }

    void TextureManager::DiscardTexture(TextureCacheIndex& index, Texture2D* texture, uint64_t fenceValue)
    {
        TextureQueue& readyQueue = m_AvailableTextures[index];
        readyQueue.push({ fenceValue, texture });
    }
    size_t TextureManager::TextureCacheHash::operator()(TextureCacheIndex const& index) const
    {
        size_t seed = 0;
        hash_combine<uint32_t>(seed, index.Width);
        hash_combine<uint32_t>(seed, index.Height);
        hash_combine<uint32_t>(seed, index.Depth);
        hash_combine<uint32_t>(seed, index.Format);
        return seed;
    }
}
