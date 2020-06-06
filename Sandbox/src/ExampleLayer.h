#pragma once

#include "Hazel.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"

#include "Hazel/ComponentSystem/GameObject.h"

#include "d3d12.h"
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Texture.h"


#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Vertex.h"
#include "DeferedTexturePass.h"
#include "BaseColorPass.h"
#include "NormalsDebugPass.h"
#include "MipMapPass.h"
#include "ClearFeedbackPass.h"


enum ExampleShaders : size_t {
	DiffuseShader = 0,
	TextureShader = 1,
	Count
};


class ExampleLayer : public Hazel::Layer
{
public:
	ExampleLayer();
	virtual ~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;

private:

	

	Hazel::PerspectiveCameraController m_CameraController;
	//Hazel::OrthographicCameraController m_CameraController;
	Hazel::D3D12Context* m_Context;

	enum Models {
		CubeModel,
		SphereModel,
		TriangleModel,
		CountModel
	};
	std::vector<Hazel::Ref<Hazel::GameObject>> m_Models;

	Hazel::Ref<Hazel::GameObject> m_SceneGO;
	Hazel::Ref<Hazel::GameObject> m_PositionalLightGO;
	Hazel::Ref<Hazel::GameObject> m_MainObject;
	Hazel::Ref<Hazel::GameObject> m_SecondaryObject;

	int m_UpdateRate;
	int m_RenderedFrames;
	bool m_RotateCube = true;

	//Lights
	glm::vec4 m_AmbientLight;
	float	  m_AmbientIntensity;

	// Mip Stuff
	float m_MaxMip = 1.0f;
	float m_BlendFactor = 0.0f;

	// Textures
	enum Textures {
		DeferredTexture,
		DiffuseTexture,
		NormalTexture,
		FeedbackTexture,
		CubeTexture,
		TriangleTexture,
		WhiteTexture,
		CountTexure
	};
	std::vector<Hazel::Ref<Hazel::D3D12Texture2D>> m_Textures;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_TextureGPUHandle; // This is to render it in ImGui without messing too much with SRV heaps

	// Render Passes
	Hazel::Ref<DeferredTexturePass> m_DeferredTexturePass;
	Hazel::Ref<BaseColorPass> m_BaseColorPass;
	Hazel::Ref<NormalsDebugPass> m_NormalsPass;
	Hazel::Ref<MipMapPass>	m_MipsPass;
	Hazel::Ref<ClearFeedbackPass>	m_ClearUAVPass;

	void LoadAssets();
	void BuildPipeline();
	void LoadTextures();
};

