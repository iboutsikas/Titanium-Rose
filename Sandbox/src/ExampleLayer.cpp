#include "ExampleLayer.h"

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/vec3.hpp"
#include "glm/glm.hpp"

#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/Buffer.h"

#include <d3d12.h>
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Shader.h"


ExampleLayer::ExampleLayer()
	: Layer("ExampleLayer"), /*m_CameraController(1280.0f / 720.0f, false)*/ m_CameraController(glm::vec3(0.0f, 0.0f, 5.0f), 90.0f, (1280.0f / 720.0f), 0.1f, 100.0f)
{
	m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());

	m_Context->DeviceResources->CommandAllocator->Reset();
	m_Context->DeviceResources->CommandList->Reset(
		m_Context->DeviceResources->CommandAllocator.Get(),
		nullptr
	);

	Vertex verts[8] = {
		{ glm::vec3(-1.0f, -1.0f, +1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },  // 0
		{ glm::vec3(-1.0f, +1.0f, +1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },  // 1
		{ glm::vec3(+1.0f, +1.0f, +1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },  // 2
		{ glm::vec3(+1.0f, -1.0f, +1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },  // 3
		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) },  // 4
		{ glm::vec3(-1.0f, +1.0f, -1.0f), glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) },  // 5
		{ glm::vec3(+1.0f, +1.0f, -1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },  // 6
		{ glm::vec3(+1.0f, -1.0f, -1.0f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f) }   // 7
	};

	//Vertex verts[8] = {
	//		// First quad
	//		{ {-0.5f,  0.5f, 0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f } },
	//		{ { 0.5f, -0.5f, 0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	//		{ {-0.5f, -0.5f, 0.5f}, { 0.0f, 1.0f, 0.0f, 1.0f } },
	//		{ { 0.5f,  0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f } },

	//		// Second quad
	//		{ {-0.75f, 0.75f, 0.3f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	//		{ { 0.0f,  0.0f,  0.3f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	//		{ {-0.75f, 0.0f,  0.3f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	//		{ { 0.0f,  0.75f, 0.3f}, { 1.0f, 0.0f, 0.0f, 1.0f } }
	//};


	m_VertexBuffer = std::dynamic_pointer_cast<Hazel::D3D12VertexBuffer>(Hazel::VertexBuffer::Create((float*)verts, _countof(verts) * sizeof(Vertex)));

	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetResource()->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = _countof(verts) * sizeof(Vertex);
	m_VertexBufferView.StrideInBytes = sizeof(Vertex);


	uint32_t indices[36] = {
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	//uint32_t indices[12] = { 
	//	0, 1, 2,
	//	0, 3, 1,
	//	4, 5, 6,
	//	4, 7, 5
	//};

	m_IndexBuffer = std::dynamic_pointer_cast<Hazel::D3D12IndexBuffer>(Hazel::IndexBuffer::Create(indices, _countof(indices)));
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetResource()->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes = sizeof(indices);


	Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->CommandList->Close());

	ID3D12CommandList* const commandLists[] = {
		m_Context->DeviceResources->CommandList.Get()
	};
	m_Context->DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	m_Context->Flush();

	m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/BasicShader.hlsl");

	// We need to get this from the VertexBuffer in generic way
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vertex::Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// We need an abstract version of this too. Should probably be tied to some kind of render pass or something
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_Context->DeviceResources->Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(glm::mat4) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	Hazel::TComPtr<ID3DBlob> rootSignatureBlob;
	Hazel::TComPtr<ID3DBlob> errorBlob;
	Hazel::D3D12::ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
	// Create the root signature.
	Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 2;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvFormats.RTFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	pipelineStateStream.pRootSignature = m_RootSignature.Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(m_Shader->GetVertexBlob().Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(m_Shader->GetFragmentBlob().Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};

	Hazel::D3D12::ThrowIfFailed(
		m_Context->DeviceResources->Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState))
	);

	// Target Texture
	{
		auto width = Hazel::Application::Get().GetWindow().GetWidth();
		auto height = Hazel::Application::Get().GetWindow().GetHeight();

		m_SRVHeap = m_Context->DeviceResources->CreateDescriptorHeap(
			m_Context->DeviceResources->Device.Get(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			1,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		);

		m_RTVHeap = m_Context->DeviceResources->CreateDescriptorHeap(
			m_Context->DeviceResources->Device.Get(),
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			1
		);

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		
		Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&textureDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			nullptr,
			IID_PPV_ARGS(&m_Texture)));
	
		m_Context->DeviceResources->Device->CreateRenderTargetView(m_Texture.Get(), nullptr, m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

		m_Context->DeviceResources->Device->CreateShaderResourceView(m_Texture.Get(), nullptr, m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

void ExampleLayer::OnAttach()
{
}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate(Hazel::Timestep ts) 
{
	m_CameraController.OnUpdate(ts);

	// renderPass->Begin();
	float angle = ts * 90.0f;
	glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
	static glm::mat4 rotMatrix = glm::mat4(1.0f);
	rotMatrix = glm::rotate(rotMatrix, glm::radians(angle), rotationAxis);
	m_ModelMatrix = glm::translate(glm::mat4(1.0f), { m_Pos[0], m_Pos[1] , m_Pos[2] }) * rotMatrix;
	
	auto cmdList = m_Context->DeviceResources->CommandList;
	cmdList->SetPipelineState(m_PipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_RootSignature.Get());

	// Clear
	Hazel::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
	Hazel::RenderCommand::Clear();

	//Hazel::Renderer::BeginScene(m_CameraController.GetCamera());	

	glm::vec4 color = { 0.1f, 0.1f, 0.1f, 1.0f };

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
		m_Context->CurrentBackBufferView(),
		m_RTVHeap->GetCPUDescriptorHandleForHeapStart()
	};
	cmdList->OMSetRenderTargets(_countof(rtvs), rtvs, false, &m_Context->DepthStencilView()); 
	m_Context
		->DeviceResources
		->CommandList
		->ClearRenderTargetView(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), glm::value_ptr(color), 0, nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cmdList->IASetIndexBuffer(&m_IndexBufferView);

	auto vp = m_CameraController.GetCamera().GetViewProjectionMatrix();

	auto mvp = vp * m_ModelMatrix;

	cmdList->SetGraphicsRoot32BitConstants(0, sizeof(glm::mat4) / 4, glm::value_ptr(mvp), 0);

	cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);
	

	//cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 4, 0);
	//cmdList->DrawInstanced(3, 1, 0, 0);
	//Hazel::RenderCommand::Clear();
	//cmdList->OMSetRenderTargets(1, &m_Context->CurrentBackBufferView(), true, &m_Context->DepthStencilView());
	//cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	//cmdList->IASetIndexBuffer(&m_IndexBufferView);
	//cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);
	Hazel::Renderer::EndScene();
}

void ExampleLayer::OnImGuiRender() 
{
	if (show_another_window)
	{
		ImGui::Begin("Controls", &show_another_window);
		ImGui::DragFloat3("Position", m_Pos, 0.05f);

		ImGui::End();
	}
}

void ExampleLayer::OnEvent(Hazel::Event& e) 
{
	m_CameraController.OnEvent(e);
}
