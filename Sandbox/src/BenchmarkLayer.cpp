#include "BenchmarkLayer.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "ModelLoader.h"
#include "Vertex.h"

#include "ImGui/imgui.h"
#include "ImGui/ImGuiHelpers.h"
#include "winpixeventruntime/pix3.h"

#define MAX_RESOURCES       100
#define MAX_RENDERTARGETS   30

static uint32_t STATIC_RESOURCES = 0;
static constexpr uint32_t MaxItemsPerSubmission = 25;
static constexpr char* ShaderPath = "assets/shaders/PbrShader.hlsl";

static D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};


void Submit(Hazel::Ref<Hazel::GameObject>& go,
    std::vector<Hazel::Ref<Hazel::GameObject>>& opaqueObjects,
    std::vector<Hazel::Ref<Hazel::GameObject>>& transparentObjects)
{
    if (go->Mesh != nullptr) 
    {
        if (go->Material->IsTransparent)
        {
            transparentObjects.push_back(go);
        }
        else
        {
            opaqueObjects.push_back(go);
        }
    }

    for (auto& c : go->children)
    {
        Submit(c, opaqueObjects, transparentObjects);
    }
}

BenchmarkLayer::BenchmarkLayer()
    : Layer("BenchmarkLayer"), 
    m_ClearColor({0.1f, 0.1f, 0.1f, 1.0f }),
    m_AmbientColor({ 1.0f, 1.0f, 1.0f }),
    m_AmbientIntensity(0.1f),
    m_CameraController(glm::vec3(0.0f, 15.0f, 0.0f), 28.0f, (1280.0f / 720.0f), 0.1f, 1000.0f)
{
    m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());
    Hazel::Application::Get().GetWindow().SetVSync(false);

    m_Path.resize(4);
    m_Path[0] = { { -122.0f, 15.0f,  42.0f }, &m_Path[1] };
    m_Path[1] = { {  122.0f, 15.0f,  42.0f }, &m_Path[2] };
    m_Path[2] = { {  122.0f, 15.0f, -42.0f }, &m_Path[3] };
    m_Path[3] = { { -122.0f, 15.0f, -42.0f }, &m_Path[0] };

    ModelLoader::TextureLibrary = &m_TextureLibrary;

    m_ResourceHeap = Hazel::CreateRef<Hazel::D3D12DescriptorHeap>(
        m_Context->DeviceResources->Device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        MAX_RESOURCES
    );

    m_RenderTargetHeap = Hazel::CreateRef<Hazel::D3D12DescriptorHeap>(
        m_Context->DeviceResources->Device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        MAX_RENDERTARGETS
    );


    Hazel::D3D12ResourceBatch batch(m_Context->DeviceResources->Device.Get());
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
        m_TextureLibrary.AddTexture(t);
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
        m_TextureLibrary.AddTexture(t);
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
        m_TextureLibrary.AddTexture(t);
    }

    auto model = ModelLoader::LoadFromFile(std::string("assets/models/sponza.obj"), batch);

    m_Scene.Entities.push_back(model);
    m_Scene.Lights.resize(MaxLights);
    m_PatrolComponents.resize(MaxLights);

    for (auto& light : m_Scene.Lights)
    {
        light.GameObject = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.glb"), batch);
        light.Range = 50.0f;
        light.Intensity = 0.5f;
        light.GameObject->Material->Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        light.GameObject->Transform.SetScale(0.2f, 0.2f, 0.2f);
        light.GameObject->Transform.SetPosition(0.0f, 0.0f, 5.0f);
    }

    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        if (i == 0) {
            m_Scene.Lights[i].GameObject->Transform.SetPosition(120.0f, 15.0f, 0.0f);
            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[2];
            m_PatrolComponents[i].Patrol = false;
        }
        else if (i == 1) {
            m_Scene.Lights[i].GameObject->Transform.SetPosition(-120.0f, 15.0f, 0.0f);
            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[0];
            m_PatrolComponents[i].Patrol = false;
        }

    }

    for (auto& a : m_TextureLibrary)
    {
        a.second->DescriptorAllocation = m_ResourceHeap->Allocate(1);
        HZ_ASSERT(a.second->DescriptorAllocation.Allocated, "Could not allocate space on the resource heap");
        ++STATIC_RESOURCES;
        m_Context->DeviceResources->Device->CreateShaderResourceView(
            a.second->GetResource(),
            nullptr,
            a.second->DescriptorAllocation.CPUHandle
        );
        a.second->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    batch.End(m_Context->DeviceResources->CommandQueue.Get()).wait();

    {
        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
        rasterizer.FrontCounterClockwise = TRUE;
        rasterizer.CullMode = D3D12_CULL_MODE_BACK;

        Hazel::D3D12Shader::PipelineStateStream pipelineStateStream;

        pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        pipelineStateStream.RTVFormats = rtvFormats;
        pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);
        auto shader = Hazel::CreateRef<Hazel::D3D12Shader>(ShaderPath, pipelineStateStream);
        Hazel::ShaderLibrary::GlobalLibrary()->Add(shader);
    }
    HZ_INFO("Layer loaded");
}

void BenchmarkLayer::OnAttach()
{
}

void BenchmarkLayer::OnDetach()
{
}

void BenchmarkLayer::OnUpdate(Hazel::Timestep ts)
{
    m_LastFrameTime = ts.GetMilliseconds();
    // Update camera
    m_CameraController.OnUpdate(ts);
    for (auto& c : m_PatrolComponents)
    {
        c.OnUpdate(ts);
    }

    {
        Hazel::D3D12ResourceBatch batch(m_Context->DeviceResources->Device);
        auto cmdList = batch.Begin();

        auto backBuffer = m_Context->GetCurrentBackBuffer();
        
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_BARRIER_FLAG_NONE);

        cmdList->ResourceBarrier(1, &barrier);

        cmdList->ClearRenderTargetView(m_Context->CurrentBackBufferView(), &m_ClearColor.x, 0, nullptr);
        cmdList->ClearDepthStencilView(m_Context->DepthStencilView(),
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
            1.0f, 0, 0, nullptr
        );

        batch.End(m_Context->DeviceResources->CommandQueue.Get()).wait();
    }

#if 1

    // "Submit phase"
    
    static std::vector<Hazel::Ref<Hazel::GameObject>> opaqueObjects;
    static std::vector<Hazel::Ref<Hazel::GameObject>> transparentObjects;
    static std::vector<std::future<void>> renderTasks;
    
    opaqueObjects.clear();
    transparentObjects.clear();
    renderTasks.clear();
    
    for (auto& obj : m_Scene.Entities)
    {
        Submit(obj, opaqueObjects, transparentObjects);
    }

    for (auto& light : m_Scene.Lights)
    {
        Submit(light.GameObject, opaqueObjects, transparentObjects);
    }


    uint32_t counter = 0;

    auto shader = Hazel::ShaderLibrary::GlobalLibrary()->GetAs<Hazel::D3D12Shader>("PbrShader");
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
        Hazel::D3D12ResourceBatch batch(m_Context->DeviceResources->Device.Get());
        auto cmdList = batch.Begin();
        //PIXBeginEvent(cmdList.Get(), PIX_COLOR(1, 0, 1), "Base Color Pass");

        Hazel::D3D12UploadBuffer<HPerObjectData> perObjectBuffer(batch, MaxItemsPerSubmission, true);
        Hazel::D3D12UploadBuffer<HPassData> passBuffer(batch, 1, true);
        passBuffer.CopyData(0, passData);

        cmdList->SetPipelineState(shader->GetPipelineState());
        cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->RSSetViewports(1, &m_Context->m_Viewport);
        cmdList->RSSetScissorRects(1, &m_Context->m_ScissorRect);
        ID3D12DescriptorHeap* const heaps[] = {
            m_ResourceHeap->GetHeap()
        };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        auto rtv = m_Context->CurrentBackBufferView();
        cmdList->OMSetRenderTargets(1, &rtv, true, &m_Context->DepthStencilView());

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
        renderTasks.push_back(batch.End(m_Context->DeviceResources->CommandQueue.Get()));
    }

    for (auto& task : renderTasks)
    {
        task.wait();
    }
#endif


}

void BenchmarkLayer::OnImGuiRender()
{
#if 1

    ImGui::Begin("Diagnostics");
    ImGui::Text("Frame Time: %.2f ms (%.2f fps)", m_LastFrameTime, 1000.0f / m_LastFrameTime);
    ImGui::Text("Loaded Textures: %d", m_TextureLibrary.TextureCount());
    ImGui::End();

    ImGui::Begin("Controls");
    ImGui::ColorEdit4("Clear Color", &m_ClearColor.x);
    ImGui::End();

    ImGui::Begin("Shader Control Center");
    static auto shaderLib = Hazel::ShaderLibrary::GlobalLibrary();
    for (const auto& [key, shader] : *shaderLib)
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
    ImGui::ColorEdit3("Color", &m_AmbientColor.x);
    ImGui::DragFloat("Intensity", &m_AmbientIntensity, 0.01f, 0.0f, 1.0f, "%.2f");


    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        Hazel::Light* light = &m_Scene.Lights[i];
        PatrolComponent* c = &m_PatrolComponents[i];

        ImGui::PushID(i);
        ImGui::Text("Point Light #%d", i+1);
        ImGui::TransformControl(&light->GameObject->Transform);
        ImGui::MaterialControl(light->GameObject->Material.get());
        ImGui::DragInt("Range", &light->Range, 1, 0, 1500);
        ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, 10.0f, "%0.2f");
        ImGui::Checkbox("Follow path", &c->Patrol);
        ImGui::PopID();
    }
    ImGui::End();
#endif
}

void BenchmarkLayer::OnEvent(Hazel::Event& e)
{
}
