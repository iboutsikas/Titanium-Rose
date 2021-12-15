#include "trpch.h"
#include "TitaniumRose/Scene/Scene.h"

#include "Platform/D3D12/D3D12Renderer.h"

Roses::Scene::~Scene()
{
    //for (auto& e : Entities)
    //{
    //    e->~GameObject();
    //}
}

void Roses::Scene::LoadEnvironment(std::string& filepath)
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

void Roses::Scene::OnUpdate(Timestep ts)
{
    for (auto e : m_Entities)
    {
        e->Update(ts);
    }
}

Roses::Ref<Roses::HGameObject> Roses::Scene::AddEntity(Ref<HGameObject> go)
{
    HZ_CORE_ASSERT(go->ID == -1, "Entity has already been added!");
    go->ID = m_EntityCounter++;
    m_Entities.push_back(go);
    return go;
}

std::vector<Roses::Ref<Roses::HGameObject>>& Roses::Scene::GetEntities()
{
    return m_Entities;
}
