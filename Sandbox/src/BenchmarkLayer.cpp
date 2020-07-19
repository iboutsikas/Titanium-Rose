#include "BenchmarkLayer.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "ModelLoader.h"
#include "Hazel/Renderer/Vertex.h"

#include "ImGui/imgui.h"
#include "ImGui/ImGuiHelpers.h"
#include "winpixeventruntime/pix3.h"

static uint32_t STATIC_RESOURCES = 0;
static constexpr uint32_t MaxItemsPerSubmission = 25;
static constexpr char* ShaderPath = "assets/shaders/PbrShader.hlsl";

BenchmarkLayer::BenchmarkLayer()
    : Layer("BenchmarkLayer"), 
    m_ClearColor({0.1f, 0.1f, 0.1f, 1.0f }),
    m_CameraController(glm::vec3(0.0f, 15.0f, 0.0f), 28.0f, (1280.0f / 720.0f), 0.1f, 1000.0f)
{
    using namespace Hazel;
    // TODO: Do this through the renderer
    D3D12Renderer::SetVCsync(false);

    m_Path.resize(4);
    m_Path[0] = { { -122.0f, 15.0f,  42.0f }, &m_Path[1] };
    m_Path[1] = { {  122.0f, 15.0f,  42.0f }, &m_Path[2] };
    m_Path[2] = { {  122.0f, 15.0f, -42.0f }, &m_Path[3] };
    m_Path[3] = { { -122.0f, 15.0f, -42.0f }, &m_Path[0] };

    ModelLoader::TextureLibrary = Hazel::D3D12Renderer::TextureLibrary;

    Hazel::D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device.Get());
    batch.Begin();

    {
        Hazel::D3D12Texture2D::TextureCreationOptions opts;
        opts.Name = L"White Texture";
        opts.Width = 1;
        opts.Height = 1;
        opts.MipLevels = 1;
        opts.Flags = D3D12_RESOURCE_FLAG_NONE;

        uint8_t white[] = { 255, 255, 255, 255 };
        auto t = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
        t->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
        t->SetData(batch, white, sizeof(white));
        t->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        D3D12Renderer::TextureLibrary->AddTexture(t);
    }

    {
        Hazel::D3D12Texture2D::TextureCreationOptions opts;
        opts.Name = L"Dummy Normal Texture";
        opts.Width = 1;
        opts.Height = 1;
        opts.MipLevels = 1;
        opts.Flags = D3D12_RESOURCE_FLAG_NONE;

        uint8_t white[] = { 128, 128, 255 };
        auto t = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
        t->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
        t->SetData(batch, white, sizeof(white));
        t->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        D3D12Renderer::TextureLibrary->AddTexture(t);
    }

    {
        Hazel::D3D12Texture2D::TextureCreationOptions opts;
        opts.Name = L"Dummy Specular Texture";
        opts.Width = 1;
        opts.Height = 1;
        opts.MipLevels = 1;
        opts.Flags = D3D12_RESOURCE_FLAG_NONE;

        uint8_t white[] = { 0, 0, 0 };
        auto t = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
        t->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
        t->SetData(batch, white, sizeof(white));
        t->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        D3D12Renderer::TextureLibrary->AddTexture(t);
    }

#define USE_SPONZA 0

#if USE_SPONZA
    auto model = ModelLoader::LoadFromFile(std::string("assets/models/sponza.fbx"), batch);
#else
    auto model = ModelLoader::LoadFromFile(std::string("assets/models/earth.fbx"), batch);
    model->Transform.SetPosition(glm::vec3(0.0f, 15.0f, -10.0f));
#endif

    m_Scene.Entities.push_back(model);
    m_Scene.Lights.resize(2);
    m_Scene.Camera = &m_CameraController.GetCamera();
    m_Scene.AmbientLight = { 1.0f, 1.0f, 1.0f };
    m_Scene.AmbientIntensity = 0.1f;

    m_PatrolComponents.resize(2);
    for (auto& light : m_Scene.Lights)
    {
        light.GameObject = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.glb"), batch);
        light.Range = 50.0f;
        light.Intensity = 1.0f;
        light.GameObject->Material->Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.GameObject->Transform.SetScale(0.2f, 0.2f, 0.2f);
        light.GameObject->Transform.SetPosition(0.0f, 0.0f, 5.0f);
    }

    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        if (i == 0) {
#if USE_SPONZA
            m_Scene.Lights[i].GameObject->Transform.SetPosition(120.0f, 15.0f, 0.0f);
#else
            m_Scene.Lights[i].GameObject->Transform.SetPosition(-17.0f, 15.0f, 0.0f);
#endif
            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[2];
            m_PatrolComponents[i].Patrol = false;
        }
        else if (i == 1) {
#if USE_SPONZA
            m_Scene.Lights[i].GameObject->Transform.SetPosition(120.0f, 15.0f, 0.0f);
#else
            m_Scene.Lights[i].GameObject->Transform.SetPosition(12.0f, 15.0f, 0.0f);
#endif
            //m_Scene.Lights[i].GameObject->Transform.SetPosition(12.0f, 15.0f, 0.0f);
            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[0];
            m_PatrolComponents[i].Patrol = false;
        }

    }
#if 1
    for (auto& a : *D3D12Renderer::TextureLibrary)
    {
        D3D12Renderer::AddStaticResource(a.second);
        a.second->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
#endif
    batch.End(D3D12Renderer::Context->DeviceResources->CommandQueue.Get()).wait();
}

void BenchmarkLayer::OnAttach()
{
}

void BenchmarkLayer::OnDetach()
{
}

void BenchmarkLayer::OnUpdate(Hazel::Timestep ts)
{
    using namespace Hazel;

    m_LastFrameTime = ts.GetMilliseconds();
    // Update camera
    m_CameraController.OnUpdate(ts);
    for (auto& c : m_PatrolComponents)
    {
        c.OnUpdate(ts);
    }

    D3D12Renderer::PrepareBackBuffer(m_ClearColor);


    D3D12Renderer::BeginScene(m_Scene);

    // "Submit phase"    
    for (auto& obj : m_Scene.Entities)
    {
        D3D12Renderer::Submit(obj);
    }

    for (auto& light : m_Scene.Lights)
    {
        D3D12Renderer::Submit(light.GameObject);
    }
    
    D3D12Renderer::RenderSubmitted();
    
    D3D12Renderer::EndScene();

#if 0

    uint32_t counter = 0;

    auto shader = D3D12Renderer::ShaderLibrary->GetAs<Hazel::D3D12Shader>("PbrShader");
    HPassData passData;
    passData.ViewProjection = m_CameraController.GetCamera().GetViewProjectionMatrix();
    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        passData.Lights[i].Position = m_Scene.Lights[i].GameObject->Transform.Position();
        passData.Lights[i].Range = m_Scene.Lights[i].Range;
        passData.Lights[i].Color = m_Scene.Lights[i].GameObject->Material->Color;
        passData.Lights[i].Intensity = m_Scene.Lights[i].Intensity;
    }
    passData.AmbientLight = m_AmbientColor;
    passData.AmbientIntensity = m_AmbientIntensity;
    passData.EyePosition = m_CameraController.GetCamera().GetPosition();

    // Render all opaques
    while (counter < opaqueObjects.size())
    {
        /*
            Albedo Texture      : 0
            Normal Texture      : 1
            Specular Texture    : 2
            PerObjectCB         : 3
            PassCB              : 4
        */
        Hazel::D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device.Get());
        auto cmdList = batch.Begin();
        //PIXBeginEvent(cmdList.Get(), PIX_COLOR(1, 0, 1), "Base Color Pass");

        Hazel::D3D12UploadBuffer<HPerObjectData> perObjectBuffer(batch, MaxItemsPerSubmission, true);
        Hazel::D3D12UploadBuffer<HPassData> passBuffer(batch, 1, true);
        passBuffer.CopyData(0, passData);

        cmdList->SetPipelineState(shader->GetPipelineState());
        cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->RSSetViewports(1, &D3D12Renderer::Context->m_Viewport);
        cmdList->RSSetScissorRects(1, &D3D12Renderer::Context->m_ScissorRect);
        ID3D12DescriptorHeap* const heaps[] = {
            m_ResourceHeap->GetHeap()
        };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        auto rtv = D3D12Renderer::Context->CurrentBackBufferView();
        cmdList->OMSetRenderTargets(1, &rtv, true, &D3D12Renderer::Context->DepthStencilView());

        // WE CANNOT CLEAR THE VIEW HERE. THIS NEEDS TO BE DONE BEFOREHAND

        cmdList->SetGraphicsRootConstantBufferView(4, passBuffer.Resource()->GetGPUVirtualAddress());



        for (size_t i = 0; i < MaxItemsPerSubmission; i++)
        {
            if (counter == opaqueObjects.size())
            {
                break;
            }

            auto& go = opaqueObjects[counter];

            if (go == nullptr) {
                continue;
            }

            if (go->Mesh == nullptr) {
                continue;
            }

            HPerObjectData objectData;
            objectData.LocalToWorld = go->Transform.LocalToWorldMatrix();
            objectData.WorldToLocal = glm::transpose(go->Transform.WorldToLocalMatrix());
            objectData.HasAlbedo = go->Material->HasAlbedoTexture;
            objectData.HasNormal = go->Material->HasNormalTexture;
            objectData.HasSpecular = go->Material->HasSpecularTexture;
            objectData.Specular = go->Material->Specular;
            objectData.Color = go->Material->Color;

            perObjectBuffer.CopyData(i, objectData);

            auto vb = go->Mesh->vertexBuffer->GetView();
            vb.StrideInBytes = sizeof(Vertex);
            auto ib = go->Mesh->indexBuffer->GetView();

            cmdList->IASetVertexBuffers(0, 1, &vb);
            cmdList->IASetIndexBuffer(&ib);
            // TODO: Bind Resource heap
            cmdList->SetGraphicsRootDescriptorTable(0, go->Material->AlbedoTexture->DescriptorAllocation.GPUHandle);
            cmdList->SetGraphicsRootDescriptorTable(1, go->Material->NormalTexture->DescriptorAllocation.GPUHandle);
            cmdList->SetGraphicsRootDescriptorTable(2, go->Material->SpecularTexture->DescriptorAllocation.GPUHandle);

            cmdList->SetGraphicsRootConstantBufferView(3,
                (perObjectBuffer.Resource()->GetGPUVirtualAddress() + perObjectBuffer.CalculateOffset(i))
            );

            cmdList->DrawIndexedInstanced(go->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);

            ++counter;
        }
        //PIXEndEvent();
        batch.TrackResource(perObjectBuffer.Resource());
        batch.TrackResource(passBuffer.Resource());
        renderTasks.push_back(batch.End(D3D12Renderer::Context->DeviceResources->CommandQueue.Get()));
    }

    for (auto& task : renderTasks)
    {
        task.wait();
    }
#endif


}

void BenchmarkLayer::OnImGuiRender()
{
    using namespace Hazel;
#if 1

    ImGui::Begin("Diagnostics");
    ImGui::Text("Frame Time: %.2f ms (%.2f fps)", m_LastFrameTime, 1000.0f / m_LastFrameTime);
    ImGui::Text("Loaded Textures: %d", D3D12Renderer::TextureLibrary->TextureCount());
    ImGui::End();

    ImGui::Begin("Controls");
    ImGui::ColorEdit4("Clear Color", &m_ClearColor.x);
    auto camera_pos = m_CameraController.GetCamera().GetPosition();
    ImGui::InputFloat3("Camera Position", &camera_pos.x);
    if (m_CameraController.GetCamera().GetPosition() != camera_pos)
        m_CameraController.GetCamera().SetPosition(camera_pos);
    ImGui::End();

    ImGui::Begin("Shader Control Center");
    
    for (const auto& [key, shader] : *D3D12Renderer::ShaderLibrary)
    {
        ImGui::PushID(shader->GetName().c_str());
        ImGui::Text(shader->GetName().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Recompile")) {
            HZ_INFO("Recompile button clicked for {}", shader->GetName());
            if (!shader->Recompile()) {
                for (auto& err : shader->GetErrors()) {
                    HZ_ERROR(err);
                }
            }
            for (auto& err : shader->GetErrors()) {
                HZ_WARN(err);
            }
        }
        ImGui::PopID();
    }
    ImGui::End();

    ImGui::Begin("Lights");
    ImGui::Text("Ambient Light");
    ImGui::ColorEdit3("Color", &m_Scene.AmbientLight.x);
    ImGui::DragFloat("Intensity", &m_Scene.AmbientIntensity, 0.01f, 0.0f, 1.0f, "%.2f");


    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        Hazel::Light* light = &m_Scene.Lights[i];
        PatrolComponent* c = &m_PatrolComponents[i];

        ImGui::PushID(i);
        ImGui::Text("Point Light #%d", i+1);
        ImGui::TransformControl(&light->GameObject->Transform);
        light->GameObject->Material->EmissiveColor = light->GameObject->Material->Color;
        ImGui::MaterialControl(light->GameObject->Material.get());
        ImGui::DragInt("Range", &light->Range, 1, 0, 1500);
        ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, 10.0f, "%0.2f");
        ImGui::Checkbox("Follow path", &c->Patrol);
        ImGui::PopID();
    }

    //if (m_Scene.Lights.size() < D3D12Renderer::MaxSupportedLights - 1)
    //{
    //    if (ImGui::Button("Add Light"))
    //    {
    //        m_Scene.Lights.push_back();
    //    }
    //}

    ImGui::End();
#endif
}

void BenchmarkLayer::OnEvent(Hazel::Event& e)
{
}
