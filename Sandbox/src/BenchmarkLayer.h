#pragma once

#include "Hazel.h"
#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"

#include "TextureLibrary.h"
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
	static constexpr size_t MaxLights = 2;

    BenchmarkLayer();
    virtual ~BenchmarkLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;
private:

	// We need to use uint32_t instead of bools here, cuz of little endianess
	// and packing
	struct alignas(16) HPerObjectData {
		glm::mat4 LocalToWorld;
		glm::mat4 WorldToLocal;
		glm::vec4 Color;
		float Specular;
		uint32_t HasAlbedo;
		uint32_t HasNormal;
		uint32_t HasSpecular;		
	};

	struct alignas(16) HPassData {
		glm::mat4 ViewProjection;
		glm::vec3 AmbientLight;
		float AmbientIntensity;
		glm::vec3 EyePosition;
		float _padding;
		struct {
			glm::vec3 Position;
			uint32_t Range;
			glm::vec3 Color;
			float Intensity;
		} Lights[MaxLights];
	};

	Hazel::Scene m_Scene;

	glm::vec4 m_ClearColor;
	glm::vec3 m_AmbientColor;
	float m_AmbientIntensity;

	Hazel::PerspectiveCameraController m_CameraController;

	Hazel::D3D12Context* m_Context;
	Hazel::Ref<Hazel::D3D12DescriptorHeap> m_ResourceHeap;
	Hazel::Ref<Hazel::D3D12DescriptorHeap> m_RenderTargetHeap;
	Hazel::Ref<Hazel::GameObject> m_Model;

	std::vector<Waypoint> m_Path;
	std::vector<PatrolComponent> m_PatrolComponents;

	TextureLibrary m_TextureLibrary;
	float m_LastFrameTime;
};

