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
	glm::vec3 m_Pos;
	Hazel::Ref<Hazel::GameObject> m_CubeGO;
	Hazel::Ref<Hazel::GameObject> m_SphereGO;
	Hazel::Ref<Hazel::GameObject> m_SceneGO;

	int m_UpdateRate;
	int m_RenderedFrames;
	bool m_RotateCube = true;

	//Lights
	glm::vec4 m_AmbientLight;
	glm::vec4 m_DirectionalLight;
	glm::vec3 m_DirectionalLightPosition;
	float	  m_AmbientIntensity;
	float	  m_Glossiness;

	std::vector<Vertex> m_CubeVertices;
	std::vector<uint32_t> m_CubeIndices;

	std::vector<Vertex> m_SphereVertices;
	std::vector<uint32_t> m_SphereIndices;

	// Meshes
	Hazel::Ref<Hazel::HMesh> m_CubeMesh;
	Hazel::Ref<Hazel::HMesh> m_SphereMesh;
	//Hazel::Ref<Hazel::D3D12VertexBuffer> m_VertexBuffer;
	//Hazel::Ref<Hazel::D3D12IndexBuffer> m_IndexBuffer;

	// Textures
	Hazel::Ref<Hazel::D3D12Texture2D> m_Texture;
	Hazel::Ref<Hazel::D3D12Texture2D> m_DiffuseTexture;
	Hazel::Ref<Hazel::D3D12Texture2D> m_WhiteTexture;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_TextureGPUHandle; // This is to render it in ImGui without messing too much with SRV heaps

	// Render Passes
	Hazel::Ref<DeferedTexturePass> m_DeferredTexturePass;
	Hazel::Ref<BaseColorPass> m_BaseColorPass;
	Hazel::Ref<NormalsDebugPass> m_NormalsPass;

	void BuildPipeline();
	void LoadTextures();
	void LoadTestCube();
	void LoadTestSphere();
	void LoadGltfTest();
};

