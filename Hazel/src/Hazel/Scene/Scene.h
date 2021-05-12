#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/ComponentSystem/Component.h"

#include "Hazel/Renderer/PerspectiveCamera.h"

namespace Hazel
{
    class D3D12TextureCube;

    struct FEnvironment
    {
        Ref<D3D12TextureCube> EnvironmentMap;
        Ref<D3D12TextureCube> IrradianceMap;
    };

    class Scene
    {
    public:
        Scene() = default;
        Scene(float exposure) : Exposure(exposure) {}
        ~Scene();
        std::vector<Ref<LightComponent>> Lights;
        PerspectiveCamera* Camera;
        float Exposure;
        FEnvironment Environment;

        void LoadEnvironment(std::string& filepath);
        void OnUpdate(Timestep ts);
        Ref<HGameObject> AddEntity(Ref<HGameObject> go);
        std::vector<Ref<HGameObject>>& GetEntities();

    private:
        std::vector<Ref<HGameObject>> m_Entities;
        uint32_t m_EntityCounter = 0;
    };
}


