#include "trpch.h"
#include "TitaniumRose/Renderer/Material.h"

namespace Roses
{
    HMaterial::HMaterial() :
        Color({ 1.0f, 1.0f, 1.0f }), EmissiveColor({0.0f, 0.0f, 0.0f}), Roughness(0.5f), Metallic(0.5f),
        HasAlbedoTexture(false), HasNormalTexture(false), HasRoughnessTexture(false), HasMetallicTexture(false),
        IsTransparent(false), AlbedoTexture(nullptr), NormalTexture(nullptr), RoughnessTexture(nullptr), MetallicTexture(nullptr),
        Name("")
    {}
}