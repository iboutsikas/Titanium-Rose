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

#include <glm/gtc/type_ptr.hpp>

#include <memory>

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
    m_Path[0] = { { -122.0f, 15.0f,  42.0f }, &m_Path[1] };
    m_Path[1] = { {  122.0f, 15.0f,  42.0f }, &m_Path[2] };
    m_Path[2] = { {  122.0f, 15.0f, -42.0f }, &m_Path[3] };
    m_Path[3] = { { -122.0f, 15.0f, -42.0f }, &m_Path[0] };

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
    m_Scene.AmbientLight = { 1.0f, 1.0f, 1.0f };
    m_Scene.AmbientIntensity = 0.1f;
    m_Scene.Exposure = 1.0f;

    m_PatrolComponents.resize(2);
    for (auto& light : m_Scene.Lights)
    {
        light.GameObject = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.glb"), batch);
        light.Range = 5.0f;
        light.Intensity = 1.0f;
        light.GameObject->Material->Color = { 1.0f, 1.0f, 1.0f };
        light.GameObject->Transform.SetScale(0.2f, 0.2f, 0.2f);
        light.GameObject->Transform.SetPosition(0.0f, 0.0f, 5.0f);
    }

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
    ImGui::AlignTextToFramePadding();

    for (const auto& [key, shader] : *D3D12Renderer::ShaderLibrary)
    {
        ImGui::PushID(shader->GetName().c_str());
        ImGui::Text(shader->GetName().c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

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
    Property("Exposure", m_Scene.Exposure, 0.0f, 5.0f);
    

    ImGui::Columns(1);
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

    ImGui::End();
#endif
}

void BenchmarkLayer::OnEvent(Hazel::Event& e)
{
}


bool BenchmarkLayer::Property(const std::string& name, bool& value)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    bool result = ImGui::Checkbox(id.c_str(), &value);

    ImGui::PopItemWidth();
    ImGui::NextColumn();

    return result;
}

void BenchmarkLayer::Property(const std::string& name, float& value, float min, float max, BenchmarkLayer::PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    ImGui::SliderFloat(id.c_str(), &value, min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void BenchmarkLayer::Property(const std::string& name, glm::vec2& value, BenchmarkLayer::PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void BenchmarkLayer::Property(const std::string& name, glm::vec2& value, float min, float max, BenchmarkLayer::PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void BenchmarkLayer::Property(const std::string& name, glm::vec3& value, BenchmarkLayer::PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void BenchmarkLayer::Property(const std::string& name, glm::vec3& value, float min, float max, BenchmarkLayer::PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::ColorProperty)
        ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
    else
        ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void BenchmarkLayer::Property(const std::string& name, glm::vec4& value, BenchmarkLayer::PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void BenchmarkLayer::Property(const std::string& name, glm::vec4& value, float min, float max, BenchmarkLayer::PropertyFlag flags)
{

    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::ColorProperty)
        ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
    else
        ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}