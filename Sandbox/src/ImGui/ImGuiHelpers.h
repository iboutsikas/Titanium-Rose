#pragma once
#include <vector>
#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/ComponentSystem/GameObject.h"
namespace ImGui {
	void TransformControl(Hazel::HTransform* transform);
	void MaterialControl(Hazel::HMaterial* material);
	void MeshSelectControl(Hazel::Ref<Hazel::GameObject>& target, std::vector<Hazel::Ref<Hazel::GameObject>>& models);
};
