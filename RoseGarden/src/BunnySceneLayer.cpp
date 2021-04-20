#include "BunnySceneLayer.h"

#include "Hazel/Mesh/ModelLoader.h"

#include "Platform/D3D12/D3D12Renderer.h"

BunnySceneLayer::BunnySceneLayer()
    : BenchmarkLayer("BunnySceneLayer")
{
#if 0
    using namespace Hazel;

    m_EnableCapture = false;

    D3D12Renderer::SetVCsync(false);

    auto r = D3D12Renderer::Context->DeviceResources.get();
    auto fr = D3D12Renderer::Context->CurrentFrameResource;

    D3D12ResourceBatch batch(r->Device, r->WorkerCommandList);
    batch.Begin(r->CommandAllocator);

    ModelLoader::LoadScene(m_Scene, std::string("assets/models/bunny_scene.fbx"), batch);

    
    auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"), batch);
    go->Transform.SetPosition(5, 50, 30);
    go->Name = "Scene Light";
    auto light = go->AddComponent<LightComponent>();
    light->Range = 1000.0f;
    light->Intensity = 1.0f;
    light->Color = { 1.0f, 1.0f, 1.0f };

    m_Scene.Entities.push_back(go);
    m_Scene.Lights.push_back(light);


    for (auto& a : *D3D12Renderer::TextureLibrary)
    {
        auto tex = std::static_pointer_cast<Texture>(a.second);

        D3D12Renderer::AddStaticResource(tex);
        a.second->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    batch.EndAndWait(D3D12Renderer::Context->DeviceResources->CommandQueue);

    m_Scene.LoadEnvironment(std::string("assets/environments/pink_sunrise_4k.hdr"));
#endif
}
