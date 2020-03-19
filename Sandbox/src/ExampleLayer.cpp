#include "ExampleLayer.h"

#include <inttypes.h>

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/vec3.hpp"
#include "glm/glm.hpp"

#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/Buffer.h"

#include "Hazel/ComponentSystem/GameObject.h"

#include <d3d12.h>
#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "tinyobjloader/tiny_obj_loader.h"
#include "tinygltf/tiny_gltf.h"

#include <unordered_map>

#define TEXTURE_WIDTH 512.0f
#define TEXTURE_HEIGHT 512.0f

static bool use_rendered_texture = false;
//extern template class Hazel::D3D12UploadBuffer<PassData>;

static D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

ExampleLayer::ExampleLayer()
	: Layer("ExampleLayer"), /*m_CameraController(1280.0f / 720.0f, false)*/ 
	m_CameraController(glm::vec3(0.0f, 0.0f, 5.0f), 28.0f, (1280.0f / 720.0f), 0.1f, 100.0f),
	m_ClearColor({ 0.1f, 0.1f, 0.1f, 1.0f }),
	m_Pos({ 0.0f, 0.0f, 12.0f}),
	m_UpdateRate(60),
	m_RenderedFrames(61),
	m_AmbientLight({1.0f, 1.0f, 1.0f, 1.0f}),
	m_DirectionalLight({1.0f, 1.0f, 1.0f, 1.0f}),
	m_AmbientIntensity(0.1f)
{
	m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());
	m_Shaders.resize(ExampleShaders::Count);

	auto secondCube = new Hazel::GameObject();
	m_CubeGO.AddChild(secondCube);
	secondCube->transform.SetPosition(glm::vec3(5.0f, 3.0f, 0.0f));

	LoadTestCube();
	//LoadGltfTest();
	BuildPipeline();
	LoadTextures();
	
	m_PassCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<PassData>>(1, true);
	m_PassCB->Resource()->SetName(L"Scene CB");

	m_PerObjectCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<PerObjectData>>(2, true);
	m_PerObjectCB->Resource()->SetName(L"Per Object CB");

	//Hazel::Application::Get().GetWindow().SetVSync(false);
}

void ExampleLayer::OnAttach()
{
	uint8_t bytes[] = { 0xde, 0xad, 0xbe, 0xef, 0x12, 0x34, 0x56, 0x78};

	uint32_t* ints = (uint32_t* )bytes;

}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate(Hazel::Timestep ts) 
{
	m_CameraController.OnUpdate(ts);

	float angle = ts * 90.0f;
	static glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
	m_CubeGO.transform.SetPosition(m_Pos);

	if (m_RotateCube) {
		m_CubeGO.transform.RotateAround(rotationAxis, angle);
	}

	auto modelMatrix = m_CubeGO.transform.LocalToWorldMatrix();
	

	
	auto cmdList = m_Context->DeviceResources->CommandList;
	
	// Clear
	Hazel::RenderCommand::SetClearColor(m_ClearColor);
	auto vp = m_CameraController.GetCamera().GetViewProjectionMatrix();

	auto mvp = vp * modelMatrix;

	PassData passCB;
	passCB.ViewProjection = vp;
	passCB.AmbientLight = m_AmbientLight;
	passCB.DirectionalLight = m_DirectionalLight;
	passCB.AmbientIntensity = m_AmbientIntensity;
	passCB.CameraPosition = m_CameraController.GetCamera().GetPosition();
	m_PassCB->CopyData(0, passCB);


	PerObjectData cube1Data;
	cube1Data.ModelMatrix = modelMatrix;
	cube1Data.NormalsMatrix = m_CubeGO.transform.RotationMatrix();

	// Texture 0 is deferred, Texture 1 is diffuse
	if (!use_rendered_texture) {
		cube1Data.TextureIndex = 1;
	}
	else {
		cube1Data.TextureIndex = 0;
	}
	m_PerObjectCB->CopyData(0, cube1Data);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cmdList->IASetIndexBuffer(&m_IndexBufferView);
	cmdList->SetDescriptorHeaps(1, m_Context->DeviceResources->SRVDescriptorHeap.GetAddressOf());
	//// look into gltf format
	// Shader reloading
	// Mipmaps
	// Basic Material

#if 1
	if (m_RenderedFrames >= m_UpdateRate) {
		m_RenderedFrames = 0;
		
		D3D12_VIEWPORT vp = {0.0f, 0.0f, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0.0f, 1.0f};
		D3D12_RECT rect = { 0.0f, 0.0f, TEXTURE_WIDTH, TEXTURE_HEIGHT};
		auto shader = m_Shaders[ExampleShaders::TextureShader];
		
		cmdList->SetPipelineState(shader->GetPipelineState());
		cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
		
		// This points to the "table" with the normal, diffuse texture
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
			2,
			m_Context->GetSRVDescriptorSize()
		);
		
		// Table is at root parameter index 1
		cmdList->SetGraphicsRootDescriptorTable(2, srvHandle);
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &rect);

		cmdList->SetGraphicsRootConstantBufferView(0, m_PassCB->Resource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(1, m_PerObjectCB->Resource()->GetGPUVirtualAddress());
		
		m_Texture->Transition(D3D12_RESOURCE_STATE_RENDER_TARGET);
		
		auto rtv = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

		cmdList->OMSetRenderTargets(1, &rtv, true, nullptr);

		m_Context
			->DeviceResources
			->CommandList
			->ClearRenderTargetView(rtv, glm::value_ptr(m_ClearColor), 0, nullptr);

		cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);

		m_Texture->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
#endif
	

	auto shader = m_Shaders[ExampleShaders::DiffuseShader];
	cmdList->SetPipelineState(shader->GetPipelineState());
	cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
	cmdList->SetGraphicsRootConstantBufferView(0, m_PassCB->Resource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, m_PerObjectCB->Resource()->GetGPUVirtualAddress());
	// This points to the "table" with texture(s)
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
		m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
		1,
		m_Context->GetSRVDescriptorSize()
	);

	// Table is at root parameter index 1
	cmdList->SetGraphicsRootDescriptorTable(2, srvHandle);
	
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cmdList->IASetIndexBuffer(&m_IndexBufferView);
	cmdList->OMSetRenderTargets(1, &m_Context->CurrentBackBufferView(), false, &m_Context->DepthStencilView());
	cmdList->RSSetViewports(1, &m_Context->m_Viewport);
	cmdList->RSSetScissorRects(1, &m_Context->m_ScissorRect);
	Hazel::RenderCommand::Clear();
	
	// Main cube
	cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);
	
	// Child cube
	auto& childCube = m_CubeGO.children[0];
	PerObjectData cube2Data;
	cube2Data.ModelMatrix = childCube->transform.LocalToWorldMatrix();
	cube2Data.NormalsMatrix = childCube->transform.RotationMatrix();
	cube2Data.TextureIndex = 1;
	m_PerObjectCB->CopyData(1, cube2Data);
	auto offset = m_PerObjectCB->CalculateOffset(1);
	cmdList->SetGraphicsRootConstantBufferView(1, m_PerObjectCB->Resource()->GetGPUVirtualAddress() + offset);
	cmdList->DrawIndexedInstanced(m_IndexBuffer->GetCount(), 1, 0, 0, 0);

	Hazel::Renderer::EndScene();
	m_RenderedFrames++;
}

void ExampleLayer::OnImGuiRender() 
{
	static bool show_diffuse = false;
	static int diffuse_dim[] = { 512, 512 };

	//ImGui::ShowMetricsWindow();

	ImGui::Begin("Controls");
	ImGui::ColorEdit4("Clear Color", &m_ClearColor.x);

	if (ImGui::CollapsingHeader("Cube")) {
		ImGui::DragFloat3("Cube Position", &m_Pos.x, 0.05f);
		ImGui::Checkbox("Rotate", &m_RotateCube);
	}

	if (ImGui::CollapsingHeader("Diffuse Texture")) {
		ImGui::Checkbox("Show Diffuse Preview", &show_diffuse);
		if (show_diffuse) {
			ImGui::InputInt2("Preview Dimensions", diffuse_dim);
		}
	}

	if (ImGui::CollapsingHeader("Rendered Texture")) {
		ImGui::InputInt("Texture update rate (frames)", &m_UpdateRate);
		ImGui::Checkbox("Use rendered texture", &use_rendered_texture);
	}
	

	if (ImGui::CollapsingHeader("Lights")) {
		ImGui::ColorEdit4("Ambient Light Color",&m_AmbientLight.x);
		ImGui::DragFloat("Ambient Light Intensity", &m_AmbientIntensity, 0.1f, 0.0f, 1.0f);
		ImGui::ColorEdit4("Directional Light Color", &m_DirectionalLight.x);
	}

	ImGui::End();
	
	ImGui::Begin("Shader Control Center");
	for (auto shader : m_Shaders)
	{
		ImGui::PushID(shader->GetName().c_str());
		ImGui::Text(shader->GetName().c_str());
		ImGui::SameLine();
		if (ImGui::Button("Recompile")) {
			HZ_INFO("Recompile button clicked for {}", shader->GetName());
			if (!shader->Recompile()) {
				for (auto& err : shader->GetErrors()) {
					HZ_ERROR(err);
				}
			}
		}
		ImGui::PopID();
	}
	ImGui::End();
	
	ImGui::Begin("Texture View");
	ImGui::Image((ImTextureID)m_TextureGPUHandle.ptr, ImVec2(TEXTURE_WIDTH, TEXTURE_HEIGHT));
	ImGui::End();

	if (show_diffuse) {
		CD3DX12_GPU_DESCRIPTOR_HANDLE handle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
			2,
			m_Context->GetSRVDescriptorSize()
		);

		ImGui::Begin("Diffuse Texture", &show_diffuse);
		ImGui::Image(
			(ImTextureID)handle.ptr, 
			/*ImVec2(m_DiffuseTexture->GetWidth(), m_DiffuseTexture->GetHeight())*/
			ImVec2(diffuse_dim[0], diffuse_dim[1])
		);
		ImGui::End();
	}
}

void ExampleLayer::OnEvent(Hazel::Event& e) 
{
	m_CameraController.OnEvent(e);
}

void ExampleLayer::LoadTextures()
{
	// Create the deferred texture and its views
	{
		auto width = TEXTURE_WIDTH; // Hazel::Application::Get().GetWindow().GetWidth();
		auto height = TEXTURE_HEIGHT; // Hazel::Application::Get().GetWindow().GetHeight();
		
		m_RTVHeap = m_Context->DeviceResources->CreateDescriptorHeap(
			m_Context->DeviceResources->Device.Get(),
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			1
		);
		
		m_Texture = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create(width, height));

		m_Context
			->DeviceResources
			->Device
			->CreateRenderTargetView(m_Texture->GetCommitedResource(), nullptr, m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			1,
			m_Context->GetSRVDescriptorSize()
		);

		m_Context->DeviceResources->Device->CreateShaderResourceView(m_Texture->GetCommitedResource(), nullptr, srvHandle);

		m_TextureGPUHandle.InitOffsetted(
			m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
			1,
			m_Context->GetSRVDescriptorSize()
		);
	}

	//Diffuse Texture
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			2,
			m_Context->GetSRVDescriptorSize()
		);

		m_Context->DeviceResources->Device->CreateShaderResourceView(m_DiffuseTexture->GetCommitedResource(), nullptr, srvHandle);
	}
}

void ExampleLayer::LoadTestCube()
{
	std::string err;
	std::string warn;
	std::string filename = "assets/models/test_cube.obj";
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());

	if (!warn.empty()) {
		HZ_WARN("Loading cube warning: {0}", warn);
	}

	if (!err.empty()) {
		HZ_ERROR("Loading cube error: {0}", err);
	}
	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];

				Vertex vertex(
					glm::vec3(vx, vy, vz),
					glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
					glm::vec3(nx, ny, nz),
					glm::vec2(tx, ty)
				);

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
					m_Vertices.push_back(vertex);
				}

				m_Indices.push_back(uniqueVertices[vertex]);
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}
	
}

void ExampleLayer::LoadGltfTest()
{

	tinygltf::Model gltf_model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	std::string filename = "assets/models/triangle.gltf";

	bool ret = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, filename);
	//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

	//GameObject go;
	//// We don't care about cameras and lights and stuff yet
	//auto node = gltf_model.nodes[0];
	//go.m_Meshes.resize(node.);
	//
	//for (uint32_t i = 0; i < gltf_model.meshes.size(); i++)
	//{
	//	go.m_Meshes[i]
	//}
}

void ExampleLayer::BuildPipeline()
{
	// Geometry Buffers
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

		m_DiffuseTexture = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create("assets/textures/Cube_texture.png"));
		m_DiffuseTexture->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		std::wstring name = L"Diffuse";
		m_DiffuseTexture->DebugNameResource(name);

		Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->CommandList->Close());

		ID3D12CommandList* const commandLists[] = {
			m_Context->DeviceResources->CommandList.Get()
		};
		m_Context->DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		m_Context->Flush();
	}
	// Diffuse shader
	{
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
		rasterizer.FrontCounterClockwise = TRUE;
		rasterizer.CullMode = D3D12_CULL_MODE_BACK;

		Hazel::D3D12Shader::PipelineStateStream pipelineStateStream;

		pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

		auto shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/BasicShader.hlsl", pipelineStateStream);
		Hazel::ShaderLibrary::GlobalLibrary()->Add(shader);
		m_Shaders[ExampleShaders::DiffuseShader] = shader;
	}

	// Texture Shader
	{
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;


		CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
		//rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		rasterizer.FrontCounterClockwise = TRUE;
		rasterizer.CullMode = D3D12_CULL_MODE_BACK;
		Hazel::D3D12Shader::PipelineStateStream pipelineStateStream;

		pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

		auto shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/TextureShader.hlsl", pipelineStateStream);
		Hazel::ShaderLibrary::GlobalLibrary()->Add(shader);
		m_Shaders[ExampleShaders::TextureShader] = shader;
	}
	
}
