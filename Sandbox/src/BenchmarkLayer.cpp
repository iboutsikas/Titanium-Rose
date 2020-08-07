#include "BenchmarkLayer.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "ModelLoader.h"
#include "Hazel/Renderer/Vertex.h"
#include "Hazel/Core/Math/Ray.h"

#include "ImGui/imgui.h"
#include "ImGui/ImGuiHelpers.h"
#include "winpixeventruntime/pix3.h"


#include <memory>
#include <random>

static uint32_t STATIC_RESOURCES = 0;
static constexpr uint32_t MaxItemsPerSubmission = 25;
static constexpr char* ShaderPath = "assets/shaders/PbrShader.hlsl";

BenchmarkLayer::BenchmarkLayer()
    : Layer("BenchmarkLayer"), 
    m_ClearColor({0.1f, 0.1f, 0.1f, 1.0f }),
    m_CameraController(glm::vec3(-16.0f, 7.0f, 16.0f), 62.0f, (1280.0f / 720.0f), 0.1f, 1000.0f)
{
    using namespace Hazel;
    
    D3D12Renderer::SetVCsync(false);

    auto& cameraTransform = m_CameraController.GetCamera().GetTransform();
    cameraTransform.RotateAround(HTransform::VECTOR_UP, 45.0f);
    cameraTransform.RotateAround(cameraTransform.Right(), 15.0f);

    m_Path.resize(4);

#if USE_SPONZA
    m_Path[0] = { { -122.0f, 15.0f,  42.0f }, &m_Path[1] };
    m_Path[1] = { {  122.0f, 15.0f,  42.0f }, &m_Path[2] };
    m_Path[2] = { {  122.0f, 15.0f, -42.0f }, &m_Path[3] };
    m_Path[3] = { { -122.0f, 15.0f, -42.0f }, &m_Path[0] };
#else
    m_Path[0] = { { -13.0f, 0.0f,  13.0f }, &m_Path[1] };
    m_Path[1] = { {  13.0f, 0.0f,  13.0f }, &m_Path[2] };
    m_Path[2] = { {  13.0f, 0.0f, -13.0f }, &m_Path[3] };
    m_Path[3] = { { -13.0f, 0.0f, -13.0f }, &m_Path[0] };
#endif

    ModelLoader::TextureLibrary = Hazel::D3D12Renderer::TextureLibrary;

    Hazel::D3D12ResourceBatch batch(D3D12Renderer::Context->DeviceResources->Device, D3D12Renderer::Context->DeviceResources->CommandAllocator);
    batch.Begin();

    //{
    //    Hazel::D3D12Texture2D::TextureCreationOptions opts;
    //    opts.Name = "White Texture";
    //    opts.Width = 1;
    //    opts.Height = 1;
    //    opts.MipLevels = 1;
    //    opts.Flags = D3D12_RESOURCE_FLAG_NONE;

    //    uint8_t white[] = { 255, 255, 255, 255 };
    //    auto t = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
    //    t->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
    //    t->SetData(batch, white, sizeof(white));
    //    t->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //    D3D12Renderer::TextureLibrary->Add(t);
    //}

    //{
    //    Hazel::D3D12Texture2D::TextureCreationOptions opts;
    //    opts.Name = "Dummy Normal Texture";
    //    opts.Width = 1;
    //    opts.Height = 1;
    //    opts.MipLevels = 1;
    //    opts.Flags = D3D12_RESOURCE_FLAG_NONE;

    //    uint8_t white[] = { 128, 128, 255 };
    //    auto t = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
    //    t->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
    //    t->SetData(batch, white, sizeof(white));
    //    t->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //    D3D12Renderer::TextureLibrary->Add(t);
    //}

    //{
    //    Hazel::D3D12Texture2D::TextureCreationOptions opts;
    //    opts.Name = "Dummy Specular Texture";
    //    opts.Width = 1;
    //    opts.Height = 1;
    //    opts.MipLevels = 1;
    //    opts.Flags = D3D12_RESOURCE_FLAG_NONE;

    //    uint8_t white[] = { 0, 0, 0 };
    //    auto t = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
    //    t->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
    //    t->SetData(batch, white, sizeof(white));
    //    t->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //    D3D12Renderer::TextureLibrary->Add(t);
    //}

#define USE_SPONZA 0

#if USE_SPONZA
    auto model = ModelLoader::LoadFromFile(std::string("assets/models/sponza.fbx"), batch);
#else
    auto model = ModelLoader::LoadFromFile(std::string("assets/models/bunny_scene.fbx"), batch);
    model->Transform.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
#endif

    m_Scene.Entities.push_back(model);
    m_Scene.Lights.resize(2);
    m_Scene.Camera = &m_CameraController.GetCamera();
    m_Scene.Exposure = 1.0f;

    m_PatrolComponents.resize(2);
    for (auto& light : m_Scene.Lights)
    {
        light.GameObject = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere2.glb"), batch);
        light.Range = 15.0f;
        light.Intensity = 1.0f;
        light.GameObject->Material->Color = { 1.0f, 1.0f, 1.0f };
        light.GameObject->Transform.SetScale(0.2f, 0.2f, 0.2f);
        light.GameObject->Transform.SetPosition(0.0f, 0.0f, 5.0f);
    }

    std::random_device rd;
    std::mt19937 generator;
    std::uniform_real_distribution<float> positionDistribution(-16.0f, std::nextafter(16, DBL_MAX));
    std::uniform_real_distribution<float> colorDistribution(0.0f, std::nextafter(1, DBL_MAX));
    std::uniform_int_distribution<int> pathDistribution(0, m_Path.size() - 1);

    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        if (i == 0) {
#if USE_SPONZA
            m_Scene.Lights[i].GameObject->Transform.SetPosition(120.0f, 15.0f, 0.0f);
#else
            m_Scene.Lights[i].GameObject->Transform.SetPosition(7.0f, 0.0f, 0.0f);
#endif
            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[2];
            m_PatrolComponents[i].Patrol = false;
        }
        else if (i == 1) {
#if USE_SPONZA
            m_Scene.Lights[i].GameObject->Transform.SetPosition(120.0f, 15.0f, 0.0f);
#else
            m_Scene.Lights[i].GameObject->Transform.SetPosition(-7.0f, 0.0f, 0.0f);
#endif
            //m_Scene.Lights[i].GameObject->Transform.SetPosition(12.0f, 15.0f, 0.0f);
            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[0];
            m_PatrolComponents[i].Patrol = false;
        }
        else
        {
            float x = positionDistribution(generator);
            float z = positionDistribution(generator);

            m_Scene.Lights[i].GameObject->Transform.SetPosition(x, 0.0f, z);

            float r = colorDistribution(generator);
            float g = colorDistribution(generator);
            float b = colorDistribution(generator);
            m_Scene.Lights[i].GameObject->Material->Color = { r, g, b };
            int path = pathDistribution(generator);

            m_PatrolComponents[i].Transform = &m_Scene.Lights[i].GameObject->Transform;
            m_PatrolComponents[i].NextWaypoint = &m_Path[path];
            m_PatrolComponents[i].Patrol = false;
        }
            

    }
#if 1
    for (auto& a : *D3D12Renderer::TextureLibrary)
    {
        auto tex = std::static_pointer_cast<D3D12Texture>(a.second);

        D3D12Renderer::AddStaticResource(tex);
        a.second->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    
#endif
    batch.End(D3D12Renderer::Context->DeviceResources->CommandQueue.Get()).wait();

    m_Scene.LoadEnvironment(std::string("assets/environments/pink_sunrise_4k.hdr"));

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

    D3D12Renderer::Context->DeviceResources->CommandAllocator->Reset();

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
    
    D3D12Renderer::RenderSkybox(m_EnvironmentLevel);
    D3D12Renderer::DoToneMapping();

    D3D12Renderer::EndScene();

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
    ImGui::Columns(2);
    //ImGui::AlignTextToFramePadding();

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
    ImGui::Columns(1);
    ImGui::End();



    ImGui::Begin("Lights");
    for (size_t i = 0; i < m_Scene.Lights.size(); i++)
    {
        Hazel::Light* light = &m_Scene.Lights[i];
        PatrolComponent* c = &m_PatrolComponents[i];

        ImGui::PushID(i);
        std::string label = "Point Light #" + std::to_string(i + 1);
        ImGui::Columns(1);
        if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_None))
        {
            ImGui::TransformControl(light->GameObject->Transform);
            light->GameObject->Material->EmissiveColor = light->GameObject->Material->Color;
            ImGui::MaterialControl(light->GameObject->Material);
            ImGui::Columns(2);
            ImGui::Property("Range", light->Range, 1, 50, ImGui::PropertyFlag::None);
            ImGui::Property("Intensity", light->Intensity, 0.0f, 15.0f, ImGui::PropertyFlag::None);
            ImGui::Property("Follow path", c->Patrol);
        }
        ImGui::PopID();
    }

    ImGui::End();

    ImGui::EntityPanel(m_Selection);
    
#endif
}

void BenchmarkLayer::OnEvent(Hazel::Event& e)
{
    Hazel::EventDispatcher dispatcher(e);

    dispatcher.Dispatch<Hazel::MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(BenchmarkLayer::OnMouseButtonPressed));
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
        {
            HZ_INFO("Raycast hit: {}", hit.GameObject->Name);
            m_Selection = hit.GameObject;
        }
        else
        {
            m_Selection = nullptr;
        }
    }

    return false;
}

