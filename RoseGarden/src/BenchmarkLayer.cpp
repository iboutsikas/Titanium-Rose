#include "BenchmarkLayer.h"

//#include <memory>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <chrono>
#include <filesystem>


#include "Hazel/Core/Application.h"
#include "Hazel/Core/Math/Ray.h"
#include "Hazel/Mesh/ModelLoader.h"
#include "Hazel/Renderer/Vertex.h"
#include "Hazel/Vendor/ImGui/ImGuiHelpers.h"

#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "Platform/D3D12/Profiler/Profiler.h"


#include "ImGui/imgui.h"
#include "winpixeventruntime/pix3.h"
#include "stb_image/stb_image_write.h"

#include "Components/PatrolComponent.h"





BenchmarkLayer::BenchmarkLayer(const std::string& name, CreationOptions options)
    : Layer(name), 
    m_ClearColor({0.1f, 0.1f, 0.1f, 1.0f }),
    m_CameraController(glm::vec3(-30.0f, 7.0f, 0.0f), 62.0f, (1280.0f / 720.0f), 0.1f, 1000.0f),
    m_CreationOptions(options)
{
    using namespace Hazel;
    auto width = Hazel::Application::Get().GetWindow().GetWidth();
    auto height = Hazel::Application::Get().GetWindow().GetHeight();

    
    m_LastFrameBuffer = D3D12Renderer::ResolveFrameBuffer();
    // TODO: Hardcoded to float. We need a way to get this out of the texture hopefully
    // 4 floats per pixel
    m_ReadbackBuffer = CreateRef<ReadbackBuffer>(width * height * 4 * sizeof(uint16_t));


    m_CameraController.GetCamera().SetProjection(static_cast<float>(width), static_cast<float>(height), 62.0f, 0.1f, 1000.0f);

    auto& cameraTransform = m_CameraController.GetCamera().GetTransform();
    cameraTransform.RotateAround(HTransform::VECTOR_UP, 90.0f);
    //cameraTransform.RotateAround(cameraTransform.Right(), 15.0f);

    //m_Scene.Lights.reserve(D3D12Renderer::MaxSupportedLights);
    m_Scene.Camera = &m_CameraController.GetCamera();
    m_Scene.Exposure = 0.8f;

    D3D12Renderer::SetVCsync(false);
    
    std::ostringstream oss;

    oss << "captures/";
    oss << m_DebugName << '/';
    m_CaptureFolder = oss.str();

}

void BenchmarkLayer::OnAttach()
{
    ImGui::RegisterRenderingFunction<Hazel::LightComponent>(ImGui::LightComponentPanel);
    ImGui::RegisterRenderingFunction<PatrolComponent>(RenderPatrolComponent);

    if (m_EnableCapture || m_CreationOptions.CaptureTiming) {
        std::filesystem::create_directories(m_CaptureFolder.c_str());
    }

    std::ostringstream oss;
    oss << m_DebugName << '-' << "Decoupled update rate: " << m_CreationOptions.UpdateRate;

    Hazel::Application::Get().GetWindow().SetTitle(oss.str().c_str());
}

void BenchmarkLayer::OnDetach()
{
    // Make sure all the write tasks finish before we exit
    for (auto& task : m_CaptureTasks)
    {
        task.wait();
    }

    //Hazel::Profiler::GlobalProfiler.DumpDatabases(m_CaptureFolder + "timings.json");
}

void BenchmarkLayer::OnUpdate(Hazel::Timestep ts)
{
    using namespace Hazel;
        
    {
        ScopedTimer timer("BenchmarkLayer::Camera update");
        m_CameraController.OnUpdate(ts);
    }
    {
        ScopedTimer timer("BenchmarkLayer::Scene update");
        m_Scene.OnUpdate(ts);
    }
}

void BenchmarkLayer::OnRender(Hazel::Timestep ts, Hazel::GraphicsContext& gfxContext)
{
    using namespace Hazel;
    auto r = D3D12Renderer::Context->DeviceResources.get();
    auto fr = D3D12Renderer::Context->CurrentFrameResource;

    m_LastFrameBuffer = D3D12Renderer::ResolveFrameBuffer();

    {
        //ScopedTimer timer("Prepare Backbuffer", r->MainCommandList);
        D3D12Renderer::PrepareBackBuffer(gfxContext, m_ClearColor);
    }

    {
        //ScopedTimer timer("Begin Scene", r->MainCommandList);
        D3D12Renderer::BeginScene(m_Scene);
    }

 
    //Profiler::BeginBlock("Submit", r->WorkerCommandList);

    for (auto& obj : m_Scene.Entities)
    {
        D3D12Renderer::Submit(obj);
    }

    //Profiler::EndBlock(r->WorkerCommandList);

    if (m_CreationOptions.UpdateRate == 0 || (D3D12Renderer::GetFrameCount() % m_CreationOptions.UpdateRate) == 0)
    {
        D3D12Renderer::ShadeDecoupled();
    }

    D3D12Renderer::ClearVirtualMaps();

    D3D12Renderer::RenderSubmitted();

    D3D12Renderer::RenderSkybox(gfxContext, m_EnvironmentLevel);
    D3D12Renderer::DoToneMapping(gfxContext);

    D3D12Renderer::EndScene();
}

void BenchmarkLayer::OnImGuiRender(Hazel::GraphicsContext& uiContext)
{
    using namespace Hazel;
#if 0
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
    ImGui::Property("Texture update rate", m_CreationOptions.UpdateRate, 0, 120, ImGui::PropertyFlag::None);
    ImGui::Columns(1);
    ImGui::End();

    ImGui::EntityPanel(m_Selection);  
#endif
}

void BenchmarkLayer::OnFrameEnd()
{
    using namespace Hazel;

    m_CaptureCounter++;
    m_FrameCounter++;

    if (m_CaptureCounter >= m_CreationOptions.CaptureRate)
    {
        m_CaptureCounter = 0;

        // Remove all the done tasks
        m_CaptureTasks.erase(std::remove_if(m_CaptureTasks.begin(), m_CaptureTasks.end(),
            [](std::shared_future<void> task) {
                return task.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }), m_CaptureTasks.end());
        auto frames = m_CapturedFrames.load();
        bool shouldTerminate = (m_EnableCapture && m_CapturedFrames > m_CreationOptions.CaptureLimit) ||
                                (m_CreationOptions.CaptureTiming && m_FrameCounter > m_CreationOptions.CaptureLimit);

        if (m_EnableCapture && m_CapturedFrames <= m_CreationOptions.CaptureLimit)
            CaptureLastFramebuffer();
        else if (shouldTerminate) {
            HZ_INFO("Terminating capture session");
            ::PostQuitMessage(0);
        }
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
#if 0
    auto [mx, my] = Input::GetMousePosition();
    //ImGui::IsAnyWindowHovered();

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
#endif
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

void BenchmarkLayer::CaptureLastFramebuffer()
{
    using namespace Hazel;
    auto framebuffer = m_LastFrameBuffer;
    auto readback = m_ReadbackBuffer;
    auto frameCounter = m_FrameCounter;
    //std::string captureFolder = m_CaptureFolder;

    std::shared_future<void> task = std::async(std::launch::async, [framebuffer, readback, this, frameCounter]()
    {
        auto width = framebuffer->ColorResource->GetWidth();
        auto height = framebuffer->ColorResource->GetHeight();

        auto r = D3D12Renderer::Context->DeviceResources.get();
        auto fr = D3D12Renderer::Context->CurrentFrameResource;

        D3D12ResourceBatch batch(r->Device, r->WorkerCommandList);
        auto myCmdList = batch.Begin(r->CommandAllocator);
        auto cmdlist = myCmdList->Get();

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
        batch.EndAndWait(D3D12Renderer::Context->DeviceResources->CommandQueue);

        // 3 floats per pixel in hdr format
        float* data = new float[width * height * 3];
        uint16_t* pixels = readback->Map<uint16_t*>();

        for (uint32_t h = 0; h < height; h++)
        {
            for (uint32_t w = 0; w < width; w++)
            {
                auto index = h * width + w;
                data[index * 3 + 0] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 0]);
                data[index * 3 + 1] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 1]);
                data[index * 3 + 2] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 2]);
                //data[index * 4 + 3] = D3D12::ConvertFromFP16toFP32(pixels[index * 4 + 3]);
            }
        }
        

        std::ostringstream oss;

        oss << this->m_CaptureFolder;
        oss << "frame-" << frameCounter;
        oss << ".hdr";
        auto filename = oss.str();

        stbi_write_hdr(filename.c_str(), width, height, 3, data);
        //stbi_write_png(filename.c_str(), width, height, 3, data, sizeof(float));


        readback->Unmap();
        delete[] data;
        this->m_CapturedFrames++;
    });

    m_CaptureTasks.push_back(std::move(task));
}

void BenchmarkLayer::AppendCapturePath(std::string& suffix)
{
    m_CaptureFolder = m_CaptureFolder.append(suffix);
}
