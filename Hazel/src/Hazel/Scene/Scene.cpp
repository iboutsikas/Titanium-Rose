#include "hzpch.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/D3D12Renderer.h"

Hazel::Scene::~Scene()
{
    //for (auto& e : Entities)
    //{
    //    e->~GameObject();
    //}
}

void Hazel::Scene::LoadEnvironment(std::string& filepath)
{
    auto [env, irradiance] = D3D12Renderer::LoadEnvironmentMap(filepath);

    if (env != nullptr && irradiance != nullptr)
    {
        if (Environment.EnvironmentMap != nullptr)
        {
            D3D12Renderer::ReleaseDynamicResource(Environment.EnvironmentMap);
        }
        if (Environment.IrradianceMap)
        {
            D3D12Renderer::ReleaseDynamicResource(Environment.IrradianceMap);
        }
        D3D12Renderer::AddDynamicResource(env);
        D3D12Renderer::AddDynamicResource(irradiance);

        Environment.EnvironmentMap = env;
        Environment.IrradianceMap = irradiance;
    }
}

void Hazel::Scene::OnUpdate(Timestep ts)
{
    for (auto e : m_Entities)
    {
        e->Update(ts);
    }
}

Hazel::Ref<Hazel::HGameObject> Hazel::Scene::AddEntity(Ref<HGameObject> go)
{
    HZ_CORE_ASSERT(go->ID == -1, "Entity has already been added!");
    go->ID = m_EntityCounter++;
    m_Entities.push_back(go);
    return go;
}

std::vector<Hazel::Ref<Hazel::HGameObject>>& Hazel::Scene::GetEntities()
{
    return m_Entities;
}
