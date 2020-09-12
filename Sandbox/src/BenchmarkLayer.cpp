#include "BenchmarkLayer.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "Hazel/Mesh/ModelLoader.h"
#include "Hazel/Renderer/Vertex.h"
#include "Hazel/Core/Math/Ray.h"

#include "ImGui/imgui.h"
#include "Hazel/Vendor/ImGui/ImGuiHelpers.h"
#include "winpixeventruntime/pix3.h"
#include "stb_image/stb_image_write.h"

#include <memory>
#include <random>
#include <thread>
#include <iomanip>
#include <ctime>
#include <sstream>

static void RenderPatrolComponent(Hazel::Ref<Hazel::Component> comp)
{
    if (ImGui::CollapsingHeader("Patrol Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Hazel::Ref<PatrolComponent> patrol = std::static_pointer_cast<PatrolComponent>(comp);

        int oldColumns = ImGui::GetColumnsCount();
        ImGui::Columns(2);
        ImGui::Property("Patrol", patrol->Patrol);
        ImGui::Property("Speed", patrol->Speed, -20.0f, 20.0f);

        ImGui::Columns(oldColumns);
    }
}


BenchmarkLayer::BenchmarkLayer()
    : Layer("BenchmarkLayer"), 
    m_ClearColor({0.1f, 0.1f, 0.1f, 1.0f }),
    m_CameraController(glm::vec3(-16.0f, 7.0f, 16.0f), 62.0f, (1280.0f / 720.0f), 0.1f, 1000.0f)
{
    using namespace Hazel;
    auto width = Hazel::Application::Get().GetWindow().GetWidth();
    auto height = Hazel::Application::Get().GetWindow().GetHeight();

    // TODO: Hardcoded to float. We need a way to get this out of the texture hopefully
    // 4 floats per pixel
    m_LastFrameBuffer = D3D12Renderer::ResolveFrameBuffer();

    m_ReadbackBuffer = CreateRef<ReadbackBuffer>(width * height * 4 * sizeof(float));


    m_CameraController.GetCamera().SetProjection(static_cast<float>(width), static_cast<float>(height), 62.0f, 0.1f, 1000.0f);

    D3D12Renderer::SetVCsync(false);

    auto& cameraTransform = m_CameraController.GetCamera().GetTransform();
    cameraTransform.RotateAround(HTransform::VECTOR_UP, 45.0f);
    cameraTransform.RotateAround(cameraTransform.Right(), 15.0f);

    m_Path.resize(4);


    m_Path[0] = { { -13.0f, 0.0f,  13.0f }, &m_Path[1] };
    m_Path[1] = { {  13.0f, 0.0f,  13.0f }, &m_Path[2] };
    m_Path[2] = { {  13.0f, 0.0f, -13.0f }, &m_Path[3] };
    m_Path[3] = { { -13.0f, 0.0f, -13.0f }, &m_Path[0] };

    Hazel::D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device, D3D12Renderer::Context->DeviceResources->CommandAllocator);
    batch.Begin();

    ModelLoader::LoadScene(m_Scene, std::string("assets/models/bunny_scene.fbx"), batch);

    m_Scene.Lights.reserve(D3D12Renderer::MaxSupportedLights);
    m_Scene.Camera = &m_CameraController.GetCamera();
    m_Scene.Exposure = 0.8f;

    m_PatrolComponents.reserve(D3D12Renderer::MaxSupportedLights);

    std::random_device rd;
    std::mt19937 generator;
    std::uniform_real_distribution<float> positionDistribution(-16.0f, std::nextafter(16, DBL_MAX));
    std::uniform_real_distribution<float> colorDistribution(0.0f, std::nextafter(1, DBL_MAX));
    std::uniform_int_distribution<int> pathDistribution(0, m_Path.size() - 1);

    for (size_t i = 0; i < D3D12Renderer::MaxSupportedLights; i++)
    {
        auto go = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.fbx"), batch);
        float x = positionDistribution(generator);
        float z = positionDistribution(generator);
        go->Transform.SetPosition(x, 0.0f, z);
        go->Transform.SetScale(0.2f, 0.2f, 0.2f);
        go->Name = "Light #" + std::to_string(i + 1);

        auto light = go->AddComponent<LightComponent>();
        light->Range = 15.0f;
        light->Intensity = 1.0f;
        float r = colorDistribution(generator);
        float g = colorDistribution(generator);
        float b = colorDistribution(generator);
        light->Color = { r, g, b };

        auto patrol = go->AddComponent<PatrolComponent>();
        if (i == 0) {

            go->Transform.SetPosition(7.0f, 0.0f, 0.0f);
            patrol->NextWaypoint = &m_Path[2];
            patrol->Patrol = false;
        }
        else if (i == 1) {

            go->Transform.SetPosition(-7.0f, 0.0f, 0.0f);
            patrol->NextWaypoint = &m_Path[0];
            patrol->Patrol = false;
        }
        else
        {
            int path = pathDistribution(generator);
            patrol->NextWaypoint = &m_Path[path];
            patrol->Patrol = true;
        }


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

void BenchmarkLayer::OnAttach()
{
    ImGui::RegisterRenderingFunction<Hazel::LightComponent>(ImGui::LightComponentPanel);
    ImGui::RegisterRenderingFunction<PatrolComponent>(RenderPatrolComponent);
}

void BenchmarkLayer::OnDetach()
{
    //m_Scene.~Scene();
}

void BenchmarkLayer::OnUpdate(Hazel::Timestep ts)
{
    using namespace Hazel;

    m_LastFrameBuffer = D3D12Renderer::ResolveFrameBuffer();

    // Update camera
    m_CameraController.OnUpdate(ts);

    m_Scene.OnUpdate(ts);

    D3D12Renderer::PrepareBackBuffer(m_ClearColor);


    D3D12Renderer::BeginScene(m_Scene);

    // "Submit phase"    
    D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device);
    auto list = batch.Begin();
    for (auto& obj : m_Scene.Entities)
    {
        D3D12Renderer::Submit(obj);
        if (obj->DecoupledComponent.UseDecoupledTexture) {
            D3D12Renderer::Submit(batch, obj);
        }
    }
    batch.End(D3D12Renderer::Context->DeviceResources->CommandQueue.Get()).wait();

    m_Accumulator++;
    if (m_Accumulator >= m_UpdateRate)
    {
        m_Accumulator = 0;
        D3D12Renderer::UpdateVirtualTextures();
        D3D12Renderer::RenderVirtualTextures();
        D3D12Renderer::GenerateVirtualMips();

    }

    D3D12Renderer::ClearVirtualMaps();
    
    D3D12Renderer::RenderSubmitted();
    
    D3D12Renderer::RenderSkybox(m_EnvironmentLevel);
    D3D12Renderer::DoToneMapping();

    D3D12Renderer::EndScene();

}

void BenchmarkLayer::OnImGuiRender()
{
    using namespace Hazel;
#if 1



    ImGui::Begin("Shader Control Center");    
    ImGui::Columns(2);
    for (const auto& [key, shader] : *D3D12Renderer::ShaderLibrary)
    {
        ImGui::PushID(shader->GetName().c_str());
        ImGui::Text(shader->GetName().c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1.0f);

        if (ImGui::Button("Recompile")) {
            HZ_INFO("Recompile button clicked for {}", shader->GetName());
            if (!shader->Recompile()) {
                for (auto& err : shader->GetErrors()) {
                    HZ_ERROR(err);
                }
            }
            for (auto& warn : shader->GetWarnings()) {
                HZ_WARN(warn);
            }
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::End();

    ImGui::Begin("Environment");
    if (ImGui::Button("Load Environment Map"))
    {
        std::string filename = Application::Get().OpenFile("*.hdr");
        if (filename != "")
            m_Scene.LoadEnvironment(filename);
    }
    ImGui::DragInt("Skybox LOD", &m_EnvironmentLevel, 0.05f, 0, m_Scene.Environment.EnvironmentMap->GetMipLevels());
    ImGui::Columns(2);
    ImGui::AlignTextToFramePadding();
    ImGui::Property("Exposure", m_Scene.Exposure, 0.0f, 5.0f);
    ImGui::Property("Texture update rate", m_UpdateRate, 0, 120, ImGui::PropertyFlag::None);
    ImGui::Columns(1);
    ImGui::End();

    ImGui::EntityPanel(m_Selection);
    
#endif
}

void BenchmarkLayer::OnFrameEnd()
{
    using namespace Hazel;

    m_FrameCounter++;
    if (m_FrameCounter >= 15)
    {
        m_FrameCounter = 0;

        std::thread write_thread([](Ref<FrameBuffer> framebuffer, Ref<ReadbackBuffer> readback) {

            auto width = framebuffer->ColorResource->GetWidth();
            auto height = framebuffer->ColorResource->GetHeight();

            D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device);
            auto cmdlist = batch.Begin();
            framebuffer->ColorResource->Transition(batch, D3D12_RESOURCE_STATE_COPY_SOURCE);

            CD3DX12_TEXTURE_COPY_LOCATION source(framebuffer->ColorResource->GetResource());

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = { 0 };
            footprint.Offset = 0;
            footprint.Footprint.Format = framebuffer->ColorResource->GetFormat();
            footprint.Footprint.Width = framebuffer->ColorResource->GetWidth();
            footprint.Footprint.Height = framebuffer->ColorResource->GetHeight();
            footprint.Footprint.Depth = framebuffer->ColorResource->GetDepth();
            footprint.Footprint.RowPitch = width * 4 * sizeof(uint16_t); // The gpu is using half floats

            CD3DX12_TEXTURE_COPY_LOCATION destination(readback->GetResource(), footprint);

            cmdlist->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
            //cmdlist->CopyBufferRegion(
            //    readback->GetResource(), 0,
            //    framebuffer->ColorResource->GetResource(), 0, 
            //    width * height * 4 * sizeof(float)
            //);
            //cmdlist->CopyResource(framebuffer->ColorResource->GetResource(), readback->GetResource());


            batch.End(D3D12Renderer::Context->DeviceResources->CommandQueue.Get()).wait();

            
            
            // 3 floats per pixel in hdri format
            float* data = new float[width * height * 4];
            uint16_t* pixels = readback->Map<uint16_t*>();
            
            for (uint32_t h = 0; h < height; h++)
            {
                for (uint32_t w = 0; w < width; w++)
                {
                    auto index = h * width + w;
                    data[index * 4 + 0] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 0]);
                    data[index * 4 + 1] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 1]);
                    data[index * 4 + 2] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 2]);
                    data[index * 4 + 3] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 3]);
                }
            }
            auto t = std::time(nullptr);
            auto tm = std::localtime(&t);

            std::ostringstream oss;

            oss << "data/test/";
            oss << std::put_time(tm, "%d%m%Y-%H%M%S");
            oss << ".hdr";

            auto filename = oss.str();

            stbi_write_hdr(filename.c_str(), width, height, 4, data);


            readback->Unmap();
            delete[] data;
        }, m_LastFrameBuffer, m_ReadbackBuffer);

        write_thread.detach();
    }
}

void BenchmarkLayer::OnEvent(Hazel::Event& e)
{
    Hazel::EventDispatcher dispatcher(e);

    dispatcher.Dispatch<Hazel::MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(BenchmarkLayer::OnMouseButtonPressed));
    dispatcher.Dispatch<Hazel::WindowResizeEvent>(HZ_BIND_EVENT_FN(BenchmarkLayer::OnResize));

    m_CameraController.OnEvent(e);
}

bool BenchmarkLayer::OnMouseButtonPressed(Hazel::MouseButtonPressedEvent& event)
{
    using namespace Hazel;

    auto [mx, my] = Input::GetMousePosition();
    ImGui::IsAnyWindowHovered();

    if (event.GetMouseButton() == HZ_MOUSE_BUTTON_LEFT && !ImGui::IsAnyWindowHovered()) {
        // Convert to clip x and y
        auto vp = D3D12Renderer::Context->Viewport;

        mx -= vp.TopLeftX;
        my -= vp.TopLeftY;

        mx = (mx / vp.Width) * 2.0f - 1.0f;
        my = ((my / vp.Height) * 2.0f - 1.0f) * -1.0f;

        auto ray = m_Scene.Camera->ScreenspaceToRay(mx, my);

        RaycastHit hit;

        if (Ray::Raycast(m_Scene, ray, hit))
        {/*
            HZ_INFO("Raycast hit: {}", hit.GameObject->Name);*/
            m_Selection = hit.GameObject;
        }
        else
        {
            m_Selection = nullptr;
        }
    }

    return false;
}

bool BenchmarkLayer::OnResize(Hazel::WindowResizeEvent& event)
{
    using namespace Hazel;
    auto width = Hazel::Application::Get().GetWindow().GetWidth();
    auto height = Hazel::Application::Get().GetWindow().GetHeight();

    // TODO: Hard coded to float. We need a way to get this out of the texture hopefully
    m_ReadbackBuffer = CreateRef<ReadbackBuffer>(width * height * sizeof(float));
    return false;
}

