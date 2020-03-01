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

#include "tiny_gltf.h"

static bool have_texture = false;

ExampleLayer::ExampleLayer()
	: Layer("ExampleLayer"), /*m_CameraController(1280.0f / 720.0f, false)*/ 
	m_CameraController(glm::vec3(0.0f, 0.0f, 5.0f), 28.0f, (1280.0f / 720.0f), 0.1f, 100.0f),
	m_ClearColor({ 0.1f, 0.1f, 0.1f, 1.0f }),
	m_Pos({ 0.0f, 0.0f, -12.0f}),
	m_UpdateRate(60),
	m_RenderedFrames(61)
{
	m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());
	LoadTestCube();
	BuildPipeline();
	BuildTexturePipeline();
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

	
	// Clear
	Hazel::RenderCommand::SetClearColor(m_ClearColor);
	auto vp = m_CameraController.GetCamera().GetViewProjectionMatrix();

	auto mvp = vp * m_ModelMatrix;

	// look into gltf format
	if (m_RenderedFrames >= m_UpdateRate) {
		m_RenderedFrames = 0;
		have_texture = true;
		
		cmdList->SetPipelineState(m_TexturePipelineState.Get());
		cmdList->SetGraphicsRootSignature(m_TextureRootSignature.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
		cmdList->IASetIndexBuffer(&m_IndexBufferView);
		cmdList->SetGraphicsRoot32BitConstants(0, sizeof(glm::mat4) / 4, glm::value_ptr(mvp), 0);

		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(m_Texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
		);
		auto rtv = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

		cmdList->OMSetRenderTargets(1, &rtv, true, nullptr);

		m_Context
			->DeviceResources
			->CommandList
			->ClearRenderTargetView(rtv, glm::value_ptr(m_ClearColor), 0, nullptr);

		cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);

		cmdList->ResourceBarrier(1, 
			&CD3DX12_RESOURCE_BARRIER::Transition(m_Texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		);
	}

	cmdList->SetPipelineState(m_PipelineState.Get());
	cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cmdList->IASetIndexBuffer(&m_IndexBufferView);
	cmdList->SetGraphicsRoot32BitConstants(0, sizeof(glm::mat4) / 4, glm::value_ptr(mvp), 0);
	cmdList->OMSetRenderTargets(1, &m_Context->CurrentBackBufferView(), false, &m_Context->DepthStencilView());
	Hazel::RenderCommand::Clear();
	

	cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);
	
	Hazel::Renderer::EndScene();
	m_RenderedFrames++;
}

void ExampleLayer::OnImGuiRender() 
{
	ImGui::Begin("Controls");
	ImGui::DragFloat3("Cube Position", &m_Pos.x, 0.05f);
	ImGui::ColorEdit4("Clear Color", &m_ClearColor.x);
	ImGui::InputInt("Texture update rate (frames)", &m_UpdateRate);
	ImGui::End();

	if (have_texture) {
		ImGui::Begin("Texture View");
		ImGui::Image((ImTextureID)m_TextureGPUHandle.ptr, ImVec2(640, 360));
		ImGui::End();
	}
}

void ExampleLayer::OnEvent(Hazel::Event& e) 
{
	m_CameraController.OnEvent(e);
}

void ExampleLayer::BuildTexturePipeline()
{
	m_TextureShader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/TextureShader.hlsl");
	// Root Signature
	{
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
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_TextureRootSignature)));
	}

	// We need to get this from the VertexBuffer in generic way
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vertex::Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(Vertex::Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vertex::UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Pipeline State 
	{
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

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

		pipelineStateStream.pRootSignature = m_TextureRootSignature.Get();
		pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(m_TextureShader->GetVertexBlob().Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(m_TextureShader->GetFragmentBlob().Get());
		pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
		pipelineStateStream.RTVFormats = rtvFormats;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		Hazel::D3D12::ThrowIfFailed(
			m_Context->DeviceResources->Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_TexturePipelineState))
		);
	}
	// Create the Texture and its Views
	{
		auto width = Hazel::Application::Get().GetWindow().GetWidth();
		auto height = Hazel::Application::Get().GetWindow().GetHeight();

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
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&m_Texture)));

		m_Context->DeviceResources->Device->CreateRenderTargetView(m_Texture.Get(), nullptr, m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			1,
			m_Context->GetSRVDescriptorSize()
		);

		m_Context->DeviceResources->Device->CreateShaderResourceView(m_Texture.Get(), nullptr, srvHandle);

		m_TextureGPUHandle.InitOffsetted(
			m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
			1,
			m_Context->GetSRVDescriptorSize()
		);
	}
}

void ExampleLayer::LoadTestCube()
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	std::string filename = "assets/models/test_cube.glb";

	//bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);
	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str()); // for binary glTF(.glb)

	if (!warn.empty()) {
		HZ_WARN("Loading cube warning: {0}", warn);
	}

	if (!err.empty()) {
		HZ_ERROR("Loading cube error: {0}", err);
	}

	
}

void ExampleLayer::BuildPipeline()
{
	m_Context->DeviceResources->CommandAllocator->Reset();
	m_Context->DeviceResources->CommandList->Reset(
		m_Context->DeviceResources->CommandAllocator.Get(),
		nullptr
	);

	m_VertexBuffer = std::dynamic_pointer_cast<Hazel::D3D12VertexBuffer>(Hazel::VertexBuffer::Create((float*)m_Vertices.data(), m_Vertices.size() * sizeof(Vertex)));

	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetResource()->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = m_Vertices.size() * sizeof(Vertex);
	m_VertexBufferView.StrideInBytes = sizeof(Vertex);

	m_IndexBuffer = std::dynamic_pointer_cast<Hazel::D3D12IndexBuffer>(Hazel::IndexBuffer::Create(m_Indices.data(), m_Indices.size()));
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetResource()->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_IndexBufferView.SizeInBytes = m_Indices.size() * sizeof(uint32_t);


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
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(Vertex::Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vertex::UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

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
}
