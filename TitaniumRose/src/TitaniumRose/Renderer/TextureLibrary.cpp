#include "trpch.h"
#include "TitaniumRose/Renderer/TextureLibrary.h"  

namespace Roses {
    TextureLibrary::TextureLibrary(const std::string& libraryRoot):
        m_RootPath(libraryRoot)
    {}

    TextureLibrary::~TextureLibrary()
    {
        m_TextureMap.clear();
    }

    const Texture* TextureLibrary::LoadFromFile(const std::string& fileName)
    {
        std::stringstream filePathStream;
        filePathStream << m_RootPath << fileName;

        std::string filePath = filePathStream.str();
        std::string extension = fileName.substr(fileName.find_last_of(".") + 1);

        HZ_CORE_ASSERT(false, "Not implemented yet");
        return nullptr;
    }

    void TextureLibrary::Add(Ref<Roses::Texture> texture)
    {
        auto thing = m_TextureMap.find(texture->GetIdentifier());

        if (thing != m_TextureMap.end())
        {
            HZ_WARN("Texture {0} was already in the map but it was re-added.", texture->GetIdentifier());
        }
        m_TextureMap[texture->GetIdentifier()] = texture;
    }

    Ref<Texture> TextureLibrary::Get(std::string& key)
    {
        Ref<Texture> ret = nullptr;

        auto thing = m_TextureMap.find(key);

        if (thing != m_TextureMap.end())
        {
            ret = thing->second;
        }


        return ret;
    }

    bool TextureLibrary::Exists(std::string& key)
    {
        auto thing = m_TextureMap.find(key);

        return thing != m_TextureMap.end();
    }
}