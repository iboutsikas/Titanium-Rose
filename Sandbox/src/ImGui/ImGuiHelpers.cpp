#include "ImGuiHelpers.h"
#include "imgui/imgui.h"

void ImGui::TransformControl(Hazel::HTransform* transform)
{
	auto pos = transform->Position();
	ImGui::InputFloat3("Position", &pos.x);
	transform->SetPosition(pos);

	auto scale = transform->Scale();
	ImGui::InputFloat3("Scale", &scale.x);
	transform->SetScale(scale);

	// TODO: Rotation
}

void ImGui::MaterialControl(Hazel::HMaterial* material)
{
	ImGui::InputFloat("Glossiness", &material->Glossines);
	ImGui::ColorEdit4("Material Color", &material->Color.x);
}
