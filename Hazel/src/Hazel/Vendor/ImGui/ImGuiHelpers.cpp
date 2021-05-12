#include "hzpch.h"
#include "Hazel/Vendor/ImGui/ImGuiHelpers.h"

#include "imgui/imgui.h"

#include "Platform/D3D12/D3D12Renderer.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ImGui
{
    std::map<Hazel::ComponentID, ImGuiRenderingFn> s_RenderingFnMap;
}


static std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(Hazel::HTransform& transform)
{
    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::decompose(transform.LocalToWorldMatrix(), scale, orientation, translation, skew, perspective);

    return { translation, orientation, scale };
}


void ImGui::TransformControl(Hazel::HTransform& transform)
{
    int oldColumns = ImGui::GetColumnsCount();
    ImGui::Columns(2);
    // ------ Translation ------
    auto translation = transform.Position();
    Property("Translation", translation, 0.0f, 0.0f, PropertyFlag::InputProperty);
    if (translation != transform.Position()) {
        transform.SetPosition(translation);
    }

    // ------ Scale ------
    auto scale = transform.Scale();
    Property("Scale", scale, 0.0f, 0.0f, PropertyFlag::InputProperty);
    if (scale != transform.Scale()) {
        transform.SetScale(scale);
    }
    ImGui::Columns(oldColumns);
}

void ImGui::MaterialControl(Hazel::Ref<Hazel::HMaterial> material)
{
    int oldColumns = ImGui::GetColumnsCount();
    ImGui::Columns(2);
    // ------ Albedo ------
    ImGui::Separator();
    bool useAlbedo = material->HasAlbedoTexture;
    Property("Use albedo texture", useAlbedo);

    if (useAlbedo != material->HasAlbedoTexture) {
        material->HasAlbedoTexture = useAlbedo;
    }

    if (!useAlbedo) {
        ImGui::Property("Albedo Color", material->Color, ImGui::PropertyFlag::ColorProperty);
    }
    else {
        float factor = 1.0f / 1024;
        auto sizeInBytes = material->AlbedoTexture->GetGPUSizeInBytes();

        float sizeInKiB = sizeInBytes * factor;
        float sizeInMiB = sizeInKiB * factor;

        ImGui::Text("Size: %d bytes (%0.2f Mb)", sizeInBytes, sizeInMiB);
    }

    // ------ Roughness ------
    ImGui::Separator();
    bool useRoughness = material->HasRoughnessTexture;
    Property("Use roughness texture", useRoughness);

    if (useRoughness != material->HasRoughnessTexture) {
        material->HasRoughnessTexture = useRoughness;
    }

    if (!useRoughness) {
        ImGui::Property("Roughness", material->Roughness, 0.0f, 1.0f, PropertyFlag::None);
    }
    else {
        float factor = 1.0f / 1024;
        auto sizeInBytes = material->RoughnessTexture->GetGPUSizeInBytes();

        float sizeInKiB = sizeInBytes * factor;
        float sizeInMiB = sizeInKiB * factor;

        ImGui::Text("Size: %d bytes (%0.2f Mb)", sizeInBytes, sizeInMiB);
    }

    // ------ Metallic ------
    ImGui::Separator();
    bool useMetallic = material->HasMetallicTexture;
    Property("Use metallic texture", useMetallic);

    if (useMetallic != material->HasMetallicTexture) {
        material->HasMetallicTexture = useMetallic;
    }

    if (!useMetallic) {
        ImGui::Property("Metallic", material->Metallic, 0.0f, 1.0f, PropertyFlag::None);
    }
    else {
        float factor = 1.0f / 1024;
        auto sizeInBytes = material->MetallicTexture->GetGPUSizeInBytes();

        float sizeInKiB = sizeInBytes * factor;
        float sizeInMiB = sizeInKiB * factor;

        ImGui::Text("Size: %d bytes (%0.2f Mb)", sizeInBytes, sizeInMiB);
    }

    // ------ Emissive ------
    ImGui::Separator();

    ImGui::Property("Emissive", material->EmissiveColor, PropertyFlag::ColorProperty);

    ImGui::Columns(oldColumns);
}

void ImGui::MeshSelectControl(Hazel::Ref<Hazel::HGameObject> target, std::vector<Hazel::Ref<Hazel::HGameObject>>& models)
{
    Hazel::Ref<Hazel::HMesh> selectedMesh = target->Mesh;
    if (ImGui::BeginCombo("##combo", target->Name.c_str()))
    {
        for (auto& m : models)
        {
            bool is_selected = selectedMesh == m->Mesh;

            if (ImGui::Selectable(m->Name.c_str(), is_selected))
            {
                selectedMesh = m->Mesh;
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (selectedMesh != target->Mesh) {
        target->Mesh = selectedMesh;
    }
}

void ImGui::DecoupledTextureControl(Hazel::DecoupledTextureComponent& component)
{
    static bool showingTexture = false;
    static Hazel::HeapAllocationDescription previewAllocation = Hazel::D3D12Renderer::s_ResourceDescriptorHeap->Allocate(1);;
    static const char* numberStrings[] = {
        "0", "1","2", "3", "4", "5", "6", "7", "8",
        "9", "10", "11", "12", "13", "14", "15", "16"
    };

    int oldColumns = ImGui::GetColumnsCount();
    ImGui::Columns(2);
    //static bool useTexture = false;
    bool useTexture = component.UseDecoupledTexture;
    bool overwrite = component.OverwriteRefreshRate;
    std::string name;

    Property("Use decoupled texture", useTexture);
    component.UseDecoupledTexture = useTexture;

    Property("Overwrite update rate", overwrite);

    if (overwrite) {
        Property("Update rate", component.UpdateFrequency, 0, 250, PropertyFlag::InputProperty);
    }
    
    component.OverwriteRefreshRate = overwrite;

    // TODO: We could notify somehow here that the texture is no longer used
    if (!useTexture)
    {
        showingTexture = false;
        goto ret;
    }

    name = component.VirtualTexture == nullptr ? "None" : component.VirtualTexture->GetIdentifier();


    ImGui::Text("Texture: %s", name.c_str());

    if (component.VirtualTexture == nullptr) goto ret;

    ImGui::NextColumn();
    if (ImGui::Button(showingTexture ? "Hide": "Show")) {
        showingTexture = !showingTexture;
    }
    auto mips = component.VirtualTexture->GetMipsUsed();
    ImGui::NextColumn();
    ImGui::Text("High-res mip: %d", mips.FinestMip);
    ImGui::NextColumn();
    ImGui::Text("Low-res mip: %d", mips.CoarsestMip);
    ImGui::NextColumn();
    float factor = 1.0f / 1024;
    auto sizeInBytes = component.VirtualTexture->GetGPUSizeInBytes();

    float sizeInKiB = sizeInBytes * factor;
    float sizeInMiB = sizeInKiB * factor;

    ImGui::Text("Virtual Size: %d bytes (%0.2f Mb)", sizeInBytes, sizeInMiB);
    ImGui::NextColumn();
    auto tilesUsed = component.VirtualTexture->GetTileUsage();
    // Magic number for D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
    sizeInBytes = tilesUsed * 65536;
    sizeInKiB = sizeInBytes * factor;
    sizeInMiB = sizeInKiB * factor;

    ImGui::Text("Real Size: %d tiles (%0.2f Mb)", tilesUsed, sizeInMiB);

    if (showingTexture)
    {
        ImGui::Begin(component.VirtualTexture->GetIdentifier().c_str(), &showingTexture);
        ImVec2 dims(component.VirtualTexture->GetWidth(), component.VirtualTexture->GetHeight());
        ImGui::Text("Preview Mip:");
        ImGui::SameLine();
        static uint32_t selectedMip = mips.FinestMip;
        static const char* current_item = numberStrings[selectedMip];
        if (ImGui::BeginCombo("##combo", current_item))
        {
            for (int n = 0; n < component.VirtualTexture->GetMipLevels(); ++n)
            {
                bool is_selected = (current_item == numberStrings[n]);

                if (ImGui::Selectable(numberStrings[n], is_selected)) {
                    current_item = numberStrings[n];
                    selectedMip = n;
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        //ImGui::SetWindowSize(ImVec2(dims.x + 50, dims.y + 15));

        Hazel::D3D12Renderer::CreateSRV(std::static_pointer_cast<Hazel::Texture>(component.VirtualTexture), previewAllocation, selectedMip);

        ImGui::Image((ImTextureID)(previewAllocation.GPUHandle.ptr), dims);

        ImGui::End();
    }

ret:
    ImGui::Columns(oldColumns);
}

void ImGui::EntityPanel(Hazel::Ref<Hazel::HGameObject> target)
{
    static constexpr char* emptyString = "";


    ImGui::Begin("Selection");
    ImGui::Text("Selected entity: %s", target == nullptr ? emptyString : target->Name.c_str());

    ImGui::AlignTextToFramePadding();

    if (target == nullptr) { goto ret; }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        TransformControl(target->Transform);
    }

    if (target->Material == nullptr) { goto ret; }
    auto& mat = target->Material;
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::MaterialControl(mat);
    }

    if (ImGui::CollapsingHeader("Decoupled Texture", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DecoupledTextureControl(target->DecoupledComponent);
    }


    for (auto component : target->Components)
    {
        auto key = component->GetTypeID();
        auto it = s_RenderingFnMap.find(key);

        if (it != s_RenderingFnMap.end())
        {
            auto func = it->second;
            func(component);
        }
    }


ret:
    ImGui::End();
}

void ImGui::LightComponentPanel(Hazel::Ref<Hazel::Component> component)
{
    if (ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Hazel::Ref<Hazel::LightComponent> light = std::static_pointer_cast<Hazel::LightComponent>(component);

        int oldColumns = ImGui::GetColumnsCount();
        ImGui::Columns(2);
        ImGui::Property("Range", light->Range, 1, 50, ImGui::PropertyFlag::None);
        ImGui::Property("Intensity", light->Intensity, 0.0f, 15.0f, ImGui::PropertyFlag::None);
        ImGui::Property("Color", light->Color, PropertyFlag::ColorProperty);
        ImGui::Columns(oldColumns);
    }
    
}


bool ImGui::Property(const std::string& name, bool& value)
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

void ImGui::Property(const std::string& name, float& value, float min, float max, ImGui::PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);


    std::string id = "##" + name;

    if ((int)flags & (int)PropertyFlag::InputProperty)
        ImGui::DragFloat(id.c_str(), &value, 0.1f, min, max);
    else
        ImGui::SliderFloat(id.c_str(), &value, min, max);
    

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void ImGui::Property(const std::string& name, int& value, int min, int max, PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::InputProperty)
        ImGui::DragInt(id.c_str(), &value, 1.0f, min, max);
    else
        ImGui::SliderInt(id.c_str(), &value, min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void ImGui::Property(const std::string& name, uint64_t& value, uint64_t min, uint64_t max, PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::InputProperty)
        ImGui::DragInt(id.c_str(), (int*)&value, 1.0f, min, max);
    else
        ImGui::SliderInt(id.c_str(), (int*)&value, min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void ImGui::Property(const std::string& name, glm::vec2& value, ImGui::PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void ImGui::Property(const std::string& name, glm::vec2& value, float min, float max, ImGui::PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void ImGui::Property(const std::string& name, glm::vec3& value, ImGui::PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void ImGui::Property(const std::string& name, glm::vec3& value, float min, float max, ImGui::PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::ColorProperty)
        ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
    else if ((int)flags & (int)PropertyFlag::InputProperty)
        ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), 0.02f, min, max);
    else
        ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void ImGui::Property(const std::string& name, glm::vec4& value, ImGui::PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void ImGui::Property(const std::string& name, glm::vec4& value, float min, float max, ImGui::PropertyFlag flags)
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
