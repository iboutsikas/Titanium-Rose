#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/PerspectiveCamera.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel
{
    struct Light
    {
        Ref<GameObject> GameObject;
        int32_t Range;
        float Intensity;
    };

    struct Environment
    {
        Ref<D3D12TextureCube> EnvironmentMap;
    };

    class Scene
    {
    public:
        std::vector<Ref<GameObject>> Entities;
        std::vector<Light> Lights;
        PerspectiveCamera* Camera;
        glm::vec3 AmbientLight;
        float AmbientIntensity;
        Environment Environment;
    };
}


