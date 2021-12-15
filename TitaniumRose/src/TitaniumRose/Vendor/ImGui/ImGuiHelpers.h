#pragma once
#include <vector>

#include <map>
#include <string>

#include "TitaniumRose/ComponentSystem/Transform.h"
#include "TitaniumRose/ComponentSystem/GameObject.h"
#include "TitaniumRose/ComponentSystem/Component.h"

namespace ImGui {
    typedef void(*ImGuiRenderingFn)(Roses::Ref<Roses::Component>);

    extern std::map<Roses::ComponentID, ImGuiRenderingFn> s_RenderingFnMap;

    enum class PropertyFlag
    {
        None = 0, ColorProperty = 1, InputProperty = 2
    };

	void MeshSelectControl(Roses::Ref<Roses::HGameObject> target, std::vector<Roses::Ref<Roses::HGameObject>>& models);
    
    //================== Components=========================
	void TransformControl(Roses::HTransform& transform);
	void MaterialControl(Roses::Ref<Roses::HMaterial> material);
    void DecoupledTextureControl(Roses::DecoupledTextureComponent& component);
	void EntityPanel(Roses::Ref<Roses::HGameObject> target);
    void LightComponentPanel(Roses::Ref<Roses::Component> component);

    // ImGui UI helpers
    bool Property(const std::string& name, bool& value);
    void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, int& value, int min = -1, int max = 1, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, uint64_t& value, uint64_t min = 0, uint64_t max = 10, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
    void Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
    void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
    void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);


    template<typename TComponent,
        typename = std::enable_if_t<std::is_base_of_v<Roses::Component, TComponent>>>
    void RegisterRenderingFunction(ImGuiRenderingFn fn) 
    {
        s_RenderingFnMap[TComponent::ID] = fn;
    }
};
