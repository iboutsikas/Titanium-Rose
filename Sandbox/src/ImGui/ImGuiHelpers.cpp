#include "ImGuiHelpers.h"
#include "imgui/imgui.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>


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

void ImGui::MaterialControl(Hazel::Ref<Hazel::HMaterial>& material)
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

    // ------ Metallic ------
    ImGui::Separator();
    bool useMetallic = material->HasMatallicTexture;
    Property("Use metallic texture", useMetallic);

    if (useMetallic != material->HasMatallicTexture) {
        material->HasMatallicTexture = useMetallic;
    }

    if (!useMetallic) {
        ImGui::Property("Metallic", material->Metallic, 0.0f, 1.0f, PropertyFlag::None);
    }

    // ------ Emissive ------
    ImGui::Separator();

    ImGui::Property("Emissive", material->EmissiveColor, PropertyFlag::ColorProperty);

    ImGui::Columns(oldColumns);
}

void ImGui::MeshSelectControl(Hazel::Ref<Hazel::GameObject>& target, std::vector<Hazel::Ref<Hazel::GameObject>>& models)
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
    int oldColumns = ImGui::GetColumnsCount();
    ImGui::Columns(2);
    //static bool useTexture = false;
    bool useTexture = component.UseDecoupledTexture;
    std::string name;

    Property("Use decoupled texture", useTexture);
    component.UseDecoupledTexture = useTexture;

    // TODO: We could notify somehow here that the texture is no longer used
    if (!useTexture)
    {
        goto ret;
    }

    name = component.VirtualTexture == nullptr ? "None" : component.VirtualTexture->GetIdentifier();


    ImGui::Text("Texture: %s", name.c_str());

    if (component.VirtualTexture == nullptr) goto ret;

    ImGui::NextColumn();
    if (ImGui::Button("Show")) {

    }
    auto mips = component.VirtualTexture->GetMipsUsed();
    ImGui::NextColumn();
    ImGui::Text("High-res mip: %d", mips.FinestMip);
    ImGui::NextColumn();
    ImGui::Text("Low-res mip: %d", mips.CoarsestMip);

ret:
    ImGui::Columns(oldColumns);
}

void ImGui::EntityPanel(Hazel::Ref<Hazel::GameObject>& target)
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

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::MaterialControl(mat);
    }

    if (ImGui::CollapsingHeader("Decoupled Texture", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DecoupledTextureControl(target->DecoupledComponent);
    }


ret:
    ImGui::End();
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
