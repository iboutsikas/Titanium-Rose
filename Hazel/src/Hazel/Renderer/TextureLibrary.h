#pragma once

#include <unordered_map>

#include "Hazel.h"
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel 
{
    class TextureLibrary
    {
    public:
        TextureLibrary() = default;
        ~TextureLibrary();

        void AddTexture(Ref<D3D12Texture2D> texture);
        Ref<Hazel::D3D12Texture2D> GetTexture(std::string& key);
        bool Exists(std::string& key);
        const size_t TextureCount() const { return m_TextureMap.size(); }

        std::unordered_map<std::string, Ref<D3D12Texture2D>>::iterator begin() { return m_TextureMap.begin(); }
        std::unordered_map<std::string, Ref<D3D12Texture2D>>::iterator end() { return m_TextureMap.end(); }

        std::unordered_map<std::string, Ref<D3D12Texture2D>>::const_iterator begin() const { return m_TextureMap.begin(); }
        std::unordered_map<std::string, Ref<D3D12Texture2D>>::const_iterator end()	const { return m_TextureMap.end(); }

    private:
        std::unordered_map<std::string, Ref<D3D12Texture2D>> m_TextureMap;
    };
}
