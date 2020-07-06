#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/PerspectiveCamera.h"

namespace Hazel
{
    struct Light
    {
        Ref<GameObject> GameObject;
        int32_t Range;
        float Intensity;
    };

    class Scene
    {
    public:
        std::vector<Ref<GameObject>> Entities;
        std::vector<Light> Lights;
        PerspectiveCamera* Camera;
        glm::vec3 AmbientLight;
        float AmbientIntensity;
    };
}


