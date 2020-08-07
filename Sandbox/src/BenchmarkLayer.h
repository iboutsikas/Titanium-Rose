#pragma once

#include "Hazel.h"
#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"

#include "Platform/D3D12/D3D12Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/compatibility.hpp"

struct Waypoint
{
	glm::vec3 Position;
	Waypoint* Next;
};

struct PatrolComponent
{
	Hazel::HTransform* Transform = nullptr;
	bool Patrol = false;
	Waypoint* NextWaypoint = nullptr;
	float Speed = 5.0f;

	void OnUpdate(Hazel::Timestep ts)
	{
		if (!Patrol || Transform == nullptr || NextWaypoint == nullptr)
		{
			return;
		}

		glm::vec3 pos = Transform->Position();
		glm::vec3 movementVector = NextWaypoint->Position - pos;

		float displacement = Speed * ts.GetSeconds();

		if (glm::length2(movementVector) > displacement * displacement)
		{
			// we move towards nextwaypoint
			glm::vec3 direction = glm::normalize(movementVector);
			glm::vec3 target_pos = pos + direction * displacement;
			//float alpha = ts.GetMilliseconds();
			//target_pos = glm::lerp(pos, target_pos, ts.GetMilliseconds());

			Transform->SetPosition(target_pos);
		}
		else
		{
			// we update the waypoint
			NextWaypoint = NextWaypoint->Next;
		}
	}

};

struct DecoupledTextureComponent {

};

class BenchmarkLayer : public Hazel::Layer
{
public:
    BenchmarkLayer();
    virtual ~BenchmarkLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;
	bool OnMouseButtonPressed(Hazel::MouseButtonPressedEvent& event);

private:

	Hazel::Scene m_Scene;

	glm::vec4 m_ClearColor;

	Hazel::PerspectiveCameraController m_CameraController;

	std::vector<Waypoint> m_Path;
	std::vector<PatrolComponent> m_PatrolComponents;

	float m_LastFrameTime;
	int m_EnvironmentLevel = 0;

	Hazel::Ref<Hazel::GameObject> m_Selection = nullptr;
};

