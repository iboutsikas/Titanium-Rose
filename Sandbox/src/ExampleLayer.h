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

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Vertex {
	glm::vec3 Position;
	glm::vec4 Color;
	glm::vec3 Normal;
	glm::vec2 UV;

	Vertex(glm::vec3 position, glm::vec4 color, glm::vec3 normal, glm::vec2 uv)
		: Position(position), Color(color), Normal(normal), UV(uv)
	{ }

	Vertex(glm::vec3 position)
		: Position(position), Color({ 1.0f, 1.0f, 1.0f, 1.0f }), Normal({ 1.0f, 1.0f, 1.0f }), UV({1.0f, 1.0f})
	{ }
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
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;

	// Common State
	Hazel::Ref<Hazel::D3D12VertexBuffer> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	Hazel::Ref<Hazel::D3D12IndexBuffer> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	// Scene State
	Hazel::TComPtr<ID3D12RootSignature> m_RootSignature;
	Hazel::TComPtr<ID3D12PipelineState> m_PipelineState;
	Hazel::Ref<Hazel::D3D12Shader> m_Shader;

	// Texture State
	Hazel::TComPtr<ID3D12RootSignature> m_TextureRootSignature;
	Hazel::TComPtr<ID3D12PipelineState> m_TexturePipelineState;
	Hazel::Ref<Hazel::D3D12Shader> m_TextureShader;
	Hazel::TComPtr<ID3D12Resource> m_Texture;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_TextureGPUHandle;
	Hazel::TComPtr<ID3D12DescriptorHeap> m_RTVHeap;



	void BuildPipeline();
	void BuildTexturePipeline();
	void LoadTestCube();
};

