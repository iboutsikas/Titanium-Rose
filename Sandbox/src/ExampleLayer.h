#pragma once

#include "Hazel.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"

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


struct PassData {
	glm::mat4 MVP;
	glm::mat4 World;
	glm::mat4 NormalsMatrix;
	glm::vec4 AmbientLight;
	glm::vec4 DirectionalLight;
	glm::vec3 CameraPosition;
	float     AmbientIntensity;
};

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
	glm::mat4 m_ModelMatrix;
	bool show_another_window;
	glm::vec4 m_ClearColor;
	glm::vec3 m_Pos;
	int m_UpdateRate;
	int m_RenderedFrames;
	bool m_RotateCube = true;

	//Lights
	glm::vec4 m_AmbientLight;
	glm::vec4 m_DirectionalLight;
	float	  m_AmbientIntensity;

	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
	std::vector<Hazel::Ref<Hazel::D3D12Shader>> m_Shaders;

	// Common State
	Hazel::Ref<Hazel::D3D12VertexBuffer> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	Hazel::Ref<Hazel::D3D12IndexBuffer> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	Hazel::Ref<Hazel::D3D12UploadBuffer<PassData>> m_PassCB;

	Hazel::Ref<Hazel::D3D12Texture2D> m_Texture;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_TextureGPUHandle;
	Hazel::TComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	Hazel::Ref<Hazel::D3D12Texture2D> m_DiffuseTexture;



	void BuildPipeline();
	void LoadTextures();
	void LoadTestCube();
	void LoadGltfTest();
};

