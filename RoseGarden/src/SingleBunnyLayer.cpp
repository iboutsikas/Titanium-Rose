#include "SingleBunnyLayer.h"

#include "Hazel/Mesh/ModelLoader.h"
#include "Hazel/Core/Log.h"

#include "Platform/D3D12/D3D12Renderer.h"
#include "Components/PatrolComponent.h"

#include <unordered_map>
#include <windows.h>

static std::vector<Waypoint> RabbitWaypoints = {
    { { 0.0f, 6.3f,  13.0f} },
    { { 0.0f, 6.3f, -13.0f} },
};

static std::vector<Waypoint> PedestalWaypoints = {
    { { 0.0f, 2.0f,  13.0f} },
    { { 0.0f, 2.0f, -13.0f} },
};

static std::vector<Waypoint> LightWaypoints = {
    { {-14.5, 7,  13} },
    { { 14.5, 7,  13} },
    { { 14.5, 7, -13} },
    { {-14.5, 7, -13} }
};

struct ExperimentSetup {
    std::string LayerName;
    std::string ScenePath;
};

std::map<std::string, ExperimentSetup> experiments = {
    { "StaticBlueChrome", { "StaticBlueChrome", "assets/models/blue_chrome_static.fbx"} },
    { "DynamicBlueChrome", { "DynamicBlueChrome", "assets/models/blue_chrome.fbx"} },

    { "StaticHeroFabric", { "StaticHeroFabric", "assets/models/hero_fabric_static.fbx"} },
    { "DynamicHeroFabric", { "DynamicHeroFabric", "assets/models/hero_fabric.fbx"} },

    { "StaticMattePlastic", { "StaticMattePlastic", "assets/models/matte_plastic_static.fbx"} },
    { "DynamicMattePlastic", { "DynamicMattePlastic", "assets/models/matte_plastic.fbx"} },
};

#define ENABLE_PATROL               1
#define ENABLE_DECOUPLED_TEXTURE    1
#define ENABLE_CAPTURE              false
#define UPDATE_RATE                 15

SingleBunnyLayer::SingleBunnyLayer(CreationOptions& opts)
    : BenchmarkLayer(opts.MaterialName, opts), m_CreationOptions(opts)
{
    using namespace Hazel;

    m_EnableCapture = opts.CaptureRate > 0;

    std::string suffix = opts.ExperimentName;

    if (opts.EnablePatrol) {
        suffix.append("/decoupled/");
    }
    else {
        suffix.append("/forward/");
    }

    AppendCapturePath(suffix);

    D3D12Renderer::SetVCsync(true);

    RabbitWaypoints[0].Next = &RabbitWaypoints[1];
    RabbitWaypoints[1].Next = &RabbitWaypoints[0];

    PedestalWaypoints[0].Next = &PedestalWaypoints[1];
    PedestalWaypoints[1].Next = &PedestalWaypoints[0];
    
    D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device, D3D12Renderer::Context->DeviceResources->CommandAllocator);
    batch.Begin();

    auto experimentSetup = experiments.find(opts.MaterialName);
    if (experimentSetup == experiments.end()) {
        HZ_ERROR("Unable to find experiment: " + opts.MaterialName);
        ::PostQuitMessage(1);
    }

    auto& experiment = experimentSetup->second;
    //ModelLoader::LoadScene(m_Scene, std::string("assets/models/space_fabric_bunny.fbx"), batch);
    //ModelLoader::LoadScene(m_Scene, std::string("assets/models/single_scene.fbx"), batch);
    ModelLoader::LoadScene(m_Scene, experiment.ScenePath, batch);


    for (auto e : m_Scene.Entities) {
        if (e->Name == "Bunny.001") {
            auto comp = e->AddComponent<PatrolComponent>();
            comp->NextWaypoint = &PedestalWaypoints[0];
            comp->Speed = 2.5f;
            if(m_CreationOptions.EnablePatrol)
                comp->Patrol = true;
        }
        else if (e->Name == "Bunny") {
            auto comp = e->AddComponent<PatrolComponent>();
            comp->NextWaypoint = &RabbitWaypoints[0];
            comp->Speed = 2.5f;
            if (m_CreationOptions.EnablePatrol)
                comp->Patrol = true;
            if(m_CreationOptions.EnableDecoupled)
                e->DecoupledComponent.UseDecoupledTexture = true;
        }
    }

    // Light 1
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"), batch);
        go->Transform.SetPosition(-14.5, 7, 13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #1";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 1.0f, 0.0f, 0.0f };
        m_Scene.Entities.push_back(go);
        m_Scene.Lights.push_back(light);
    }
    
    // Light 2
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"), batch);
        go->Transform.SetPosition(14.5, 7, 13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #2";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 0.0f, 1.0f, 0.0f };
        m_Scene.Entities.push_back(go);
        m_Scene.Lights.push_back(light);
    }

    // Light 3
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"), batch);
        go->Transform.SetPosition(14.5, 7, -13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #3";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 1.0f, 1.0f, 0.0f };
        m_Scene.Entities.push_back(go);
        m_Scene.Lights.push_back(light);
    }

    // Light 4
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"), batch);
        go->Transform.SetPosition(-14.5, 7, -13);
        go->Transform.SetScale(0.3f, 0.3f, 0.3f);
        go->Name = "Scene Light #4";
        auto light = go->AddComponent<LightComponent>();
        light->Range = 30.0f;
        light->Intensity = 1.0f;
        light->Color = { 1.0f, 0.0f, 1.0f };
        m_Scene.Entities.push_back(go);
        m_Scene.Lights.push_back(light);
    }



    for (auto& a : *D3D12Renderer::TextureLibrary)
    {
        auto tex = std::static_pointer_cast<D3D12Texture>(a.second);

        D3D12Renderer::AddStaticResource(tex);
        a.second->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    batch.End(D3D12Renderer::Context->DeviceResources->CommandQueue.Get()).wait();

    m_Scene.LoadEnvironment(std::string("assets/environments/pink_sunrise_4k.hdr"));

}
