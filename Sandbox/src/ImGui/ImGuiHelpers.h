#pragma once
#include <vector>
#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/ComponentSystem/GameObject.h"
namespace ImGui {

    enum class PropertyFlag
    {
        None = 0, ColorProperty = 1, InputProperty = 2
    };

	void TransformControl(Hazel::HTransform& transform);
	void MaterialControl(Hazel::Ref<Hazel::HMaterial>& material);
	void MeshSelectControl(Hazel::Ref<Hazel::GameObject>& target, std::vector<Hazel::Ref<Hazel::GameObject>>& models);
    void DecoupledTextureControl(Hazel::DecoupledTextureComponent& component);

	void EntityPanel(Hazel::Ref<Hazel::GameObject>& target);

    // ImGui UI helpers
    bool Property(const std::string& name, bool& value);
    void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, int& value, int min = -1, int max = 1, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
    void Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
    void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
    void Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
    void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
};
