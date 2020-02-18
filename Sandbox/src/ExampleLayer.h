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

	Vertex(glm::vec3 position, glm::vec4 color)
		: Position(position), Color(color)
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
	Hazel::D3D12Context* m_Context;
	glm::mat4 m_ModelMatrix;
	bool show_another_window;
	Hazel::Ref<Hazel::D3D12VertexBuffer> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	Hazel::Ref<Hazel::D3D12IndexBuffer> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	Hazel::Ref<Hazel::D3D12Shader> m_Shader;
	Hazel::TComPtr<ID3D12RootSignature> m_RootSignature;
	Hazel::TComPtr<ID3D12PipelineState> m_PipelineState;

	float m_Pos[3] = { 0.0f, 0.0f, -2.5f };
};

