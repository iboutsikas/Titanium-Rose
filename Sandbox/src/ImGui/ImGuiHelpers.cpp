#include "ImGuiHelpers.h"
#include "imgui/imgui.h"

void ImGui::TransformControl(Hazel::HTransform* transform)
{
	auto pos = transform->Position();
	ImGui::DragFloat3("Position", &pos.x);
	transform->SetPosition(pos);

	auto scale = transform->Scale();
	ImGui::InputFloat3("Scale", &scale.x);
	transform->SetScale(scale);

	// TODO: Rotation
}

void ImGui::MaterialControl(Hazel::HMaterial* material)
{
	//ImGui::InputFloat("Specular", &material->Specular);
	ImGui::ColorEdit3("Material Color", &material->Color.x);
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
