#include "TimingBenchmarkLayer.h"

#include "Hazel/Mesh/ModelLoader.h"
#include "Hazel/Core/Log.h"

#include "Platform/D3D12/D3D12Renderer.h"
#include "Components/PatrolComponent.h"

#include <unordered_map>
#include <windows.h>

#include "nlohmann/json.hpp"

TimingBenchmarkLayer::TimingBenchmarkLayer(CreationOptions& opts)
    : BenchmarkLayer(opts.ExperimentGroup, opts), m_CreationOptions(opts)
{
    using namespace Hazel;
    using nlohmann::json;

    m_EnableCapture = opts.CaptureRate > 0;

    std::string suffix = opts.ExperimentName;

    if (opts.EnableDecoupled) {
        suffix.append("/decoupled/");
    }
    else {
        suffix.append("/forward/");
    }

    AppendCapturePath(suffix);

    D3D12Renderer::SetVCsync(false);

    auto& cameraTransform = m_CameraController.GetCamera().GetTransform();
    cameraTransform.SetPosition(-80.0f, 50.0f, 0.0f);
    cameraTransform.RotateAround(cameraTransform.Right(), 35.0f);

    const float offset = 7.0f;
    const int bound = 4;
    const int num_bunnies = (2 * bound) * (2 * bound);

    auto meshesPerFrame = std::ceil((float)num_bunnies / m_CreationOptions.UpdateRate);

    D3D12Renderer::SetPerFrameDecoupledCap(meshesPerFrame);
#if 1
    auto mesh = ModelLoader::LoadFromFile(opts.Scene);
    for (int z = -bound; z < bound; z++) {
        for (int x = -bound; x < bound; x++) {
            auto go = CreateRef<HGameObject>();

            char buffer[50];
            auto length = sprintf_s(buffer, 50, "Bunny (%d,%d)", x, z);

            go->Name = std::string(buffer, buffer + length);

            go->Transform.SetPosition(x * offset, 0, z * offset);

            go->Mesh = mesh->Mesh;

            go->Material = mesh->Material->MakeInstanceCopy();

            if (opts.EnableDecoupled) {
                go->DecoupledComponent.UseDecoupledTexture = true;
                go->DecoupledComponent.LastFrameUpdated = 0;
                go->DecoupledComponent.UpdateFrequency = m_CreationOptions.UpdateRate;
                D3D12Renderer::CreateVirtualTexture(*go);
            }
            m_Scene.AddEntity(go);
        }
    }
#endif    

    // Plane
    auto plane = ModelLoader::LoadFromFile(std::string("assets/models/plane.fbx"));
    plane->Transform.SetScale(35, 1, 35);
    plane->Material->Roughness = 1.0f;
    plane->Material->Metallic = 0.0f;
    plane->Material->IncludeAllLights = true;
    m_Scene.AddEntity(plane);

    // Lights
#if 1
    std::ifstream in("assets/lights.json");
    json j;
    in >> j;
    uint32_t index = 0;

    for (auto light : j) {
        auto p = light["position"];
        auto c = light["color"];

        glm::vec3 position = { p[0], p[1], p[2] };
        glm::vec3 color = { c[0], c[1], c[2]  };

        auto go = CreateRef<HGameObject>();
        go->Material = CreateRef<HMaterial>();
        //auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"));
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Transform.SetPosition(position);
        go->Name = "Scene Light #" + std::to_string(index);
        auto light = go->AddComponent<LightComponent>();
        light->Range = 6.5f;
        light->Intensity = 1.0f;
        light->Color = color;
        m_Scene.AddEntity(go);
        m_Scene.Lights.push_back(light);
        index++;
    }
#endif
    
#if 0
    // Light 1
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"));
        go->Transform.SetPosition(-14.5, 7, 13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #1";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 1.0f, 0.0f, 0.0f };
        m_Scene.AddEntity(go);
        m_Scene.Lights.push_back(light);
    }
    
    // Light 2
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"));
        go->Transform.SetPosition(14.5, 7, 13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #2";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 0.0f, 1.0f, 0.0f };
        m_Scene.AddEntity(go);
        m_Scene.Lights.push_back(light);
    }

    // Light 3
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"));
        go->Transform.SetPosition(14.5, 7, -13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #3";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 1.0f, 1.0f, 0.0f };
        m_Scene.AddEntity(go);
        m_Scene.Lights.push_back(light);
    }

    // Light 4
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"));
        go->Transform.SetPosition(-14.5, 7, -13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #4";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 1.0f, 0.0f, 1.0f };
        m_Scene.AddEntity(go);
        m_Scene.Lights.push_back(light);
    }
#endif


    GraphicsContext& gfxContext = GraphicsContext::Begin();
    for (auto& a : *D3D12Renderer::g_TextureLibrary)
    {
        auto tex = std::static_pointer_cast<Texture>(a.second);

        D3D12Renderer::AddStaticResource(tex);
        gfxContext.TransitionResource(*a.second, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    gfxContext.Finish(true);

    m_Scene.LoadEnvironment(std::string("assets/environments/pink_sunrise_4k.hdr"));
}
