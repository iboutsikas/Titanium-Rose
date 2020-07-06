#include "hzpch.h"
#include "Hazel/Renderer/TextureLibrary.h"  

#include <codecvt>

namespace Hazel {
    TextureLibrary::~TextureLibrary()
    {
        m_TextureMap.clear();
    }

    void TextureLibrary::AddTexture(Ref<Hazel::D3D12Texture2D> texture)
    {
        auto thing = m_TextureMap.find(texture->GetIdentifier());

        if (thing != m_TextureMap.end())
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cv;
            std::string str;
            str = cv.to_bytes(texture->GetIdentifier());

            HZ_WARN("Texture {0} was already in the map but it was re-added.", str);
        }
        m_TextureMap[texture->GetIdentifier()] = texture;
    }

    Ref<D3D12Texture2D> TextureLibrary::GetTexture(std::wstring& key)
    {
        Ref<D3D12Texture2D> ret = nullptr;

        auto thing = m_TextureMap.find(key);

        if (thing != m_TextureMap.end())
        {
            ret = thing->second;
        }


        return ret;
    }

    bool TextureLibrary::Exists(std::wstring& key)
    {
        auto thing = m_TextureMap.find(key);

        return thing != m_TextureMap.end();
    }
}