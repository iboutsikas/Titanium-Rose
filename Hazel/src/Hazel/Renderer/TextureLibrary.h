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
        TextureLibrary(const std::string& libraryRoot);
        ~TextureLibrary();
        
        const Texture* LoadFromFile(const std::string& fileName);

        void Add(Ref<Texture> texture);
        Ref<Hazel::Texture> Get(std::string& key);

        bool Exists(std::string& key);
        const size_t TextureCount() const { return m_TextureMap.size(); }

        template<typename T,
            typename = std::enable_if_t<std::is_base_of_v<Hazel::Texture, T>>>
            Ref<T> GetAs(std::string& key)
        {
            return std::static_pointer_cast<T>(Get(key));
        }

        std::unordered_map<std::string, Ref<Texture>>::iterator begin() { return m_TextureMap.begin(); }
        std::unordered_map<std::string, Ref<Texture>>::iterator end() { return m_TextureMap.end(); }

        std::unordered_map<std::string, Ref<Texture>>::const_iterator begin() const { return m_TextureMap.begin(); }
        std::unordered_map<std::string, Ref<Texture>>::const_iterator end()	const { return m_TextureMap.end(); }

    private:
        std::unordered_map<std::string, Ref<Texture>> m_TextureMap;
        std::string m_RootPath;
    };
}
