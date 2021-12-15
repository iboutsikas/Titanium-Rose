#include "Components/PatrolComponent.h"

#include "TitaniumRose/Vendor/ImGui/ImGuiHelpers.h"

#include "imgui/imgui.h"

void RenderPatrolComponent(Roses::Ref<Roses::Component> comp)
{
    if (ImGui::CollapsingHeader("Patrol Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Roses::Ref<PatrolComponent> patrol = std::static_pointer_cast<PatrolComponent>(comp);

        int oldColumns = ImGui::GetColumnsCount();
        ImGui::Columns(2);
        ImGui::Property("Patrol", patrol->Patrol);
        ImGui::Property("Speed", patrol->Speed, -20.0f, 20.0f);

        ImGui::Columns(oldColumns);
    }
}