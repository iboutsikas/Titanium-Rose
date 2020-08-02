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
	float Speed = 0.03f;

	void OnUpdate(Hazel::Timestep ts)
	{
		if (!Patrol || Transform == nullptr || NextWaypoint == nullptr)
		{
			return;
		}

		glm::vec3 pos = Transform->Position();
		glm::vec3 distance = NextWaypoint->Position - pos;

		float displacement = Speed * ts.GetMilliseconds();

		if (glm::length2(distance) > displacement * displacement)
		{
			// we move towards nextwaypoint
			glm::vec3 dir = glm::normalize(distance);
			glm::vec3 target_pos = pos + dir * displacement;
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

class BenchmarkLayer : public Hazel::Layer
{
public:

	enum class PropertyFlag
	{
		None = 0, ColorProperty = 1
	};

    BenchmarkLayer();
    virtual ~BenchmarkLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;

	// ImGui UI helpers
	bool Property(const std::string& name, bool& value);
	void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	void Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
	void Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	void Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
	void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
	void Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
	void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
private:

	Hazel::Scene m_Scene;

	glm::vec4 m_ClearColor;

	Hazel::PerspectiveCameraController m_CameraController;

	std::vector<Waypoint> m_Path;
	std::vector<PatrolComponent> m_PatrolComponents;

	float m_LastFrameTime;
	int m_EnvironmentLevel = 0;
};

