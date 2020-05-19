
#include "ExampleLayer.h"

#include <inttypes.h>

#include "imgui/imgui.h"
#include "ImGui/ImGuiHelpers.h"

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

#include"ModelLoader.h"

#include <unordered_map>

#define TEXTURE_WIDTH 4096.0f
#define TEXTURE_HEIGHT 2048.0f

static bool use_rendered_texture = true;
static bool show_rendered_texture = false;
static bool show_mip_tiles = false;
static bool render_normals = false;
//extern template class Hazel::D3D12UploadBuffer<PassData>;


static D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

ExampleLayer::ExampleLayer()
	: Layer("ExampleLayer"), /*m_CameraController(1280.0f / 720.0f, false)*/
	m_CameraController(glm::vec3(0.0f, 0.0f, 30.0f), 28.0f, (1280.0f / 720.0f), 0.1f, 100.0f),
	m_UpdateRate(15),
	m_RenderedFrames(61),
	m_AmbientLight({ 1.0f, 1.0f, 1.0f, 1.0f }),
	m_AmbientIntensity(0.1f),
	m_RotateCube(false)
{	
	m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());

	m_Models.resize(Models::CountModel);
	m_Textures.resize(Textures::CountTexure);

	LoadAssets();
	LoadTextures();
	BuildPipeline();

	// Scene Root
	m_SceneGO = Hazel::CreateRef<Hazel::GameObject>();
	m_SceneGO->Transform.SetPosition(glm::vec3({ 0.0f, 0.0f, 0.0f }));
	m_SceneGO->Name = "Scene Root";
	// Cube #1
	m_MainObject = Hazel::CreateRef<Hazel::GameObject>();
	m_MainObject->Transform.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	m_MainObject->Transform.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
	m_MainObject->Name = "Main Object";
	m_MainObject->Mesh = m_Models[Models::SphereModel]->Mesh;
	m_MainObject->Material = Hazel::CreateRef<Hazel::HMaterial>();
	m_MainObject->Material->Glossines = 32.0f;
	m_MainObject->Material->DiffuseTexture = m_Textures[Textures::DeferredTexture];
	m_MainObject->Material->NormalMap = m_Textures[Textures::NormalTexture];
	m_MainObject->Material->Color = glm::vec4(1.0f);
	m_SceneGO->AddChild(m_MainObject);
	// Cube #2
	m_SecondaryObject = Hazel::CreateRef<Hazel::GameObject>();
	m_SecondaryObject->Name = "Cube#2";
	m_SecondaryObject->Transform.SetPosition(glm::vec3(0.0f, 1.0f, 0.0f));
	m_SecondaryObject->Mesh = m_Models[Models::TriangleModel]->Mesh;
	m_SecondaryObject->Material = Hazel::CreateRef<Hazel::HMaterial>();
	m_SecondaryObject->Material->Glossines = 2.0f;
	m_SecondaryObject->Material->DiffuseTexture = m_Textures[Textures::TriangleTexture];
	m_SecondaryObject->Material->Color = glm::vec4(1.0f);
	m_MainObject->AddChild(m_SecondaryObject);

	// "Light" Sphere
	m_PositionalLightGO = Hazel::CreateRef<Hazel::GameObject>();
	m_PositionalLightGO->Name = "Light Sphere";
	m_PositionalLightGO->Mesh = m_Models[Models::SphereModel]->Mesh;
	m_PositionalLightGO->Transform.SetPosition({ 1.0, 8.0, 8.0 });
	m_PositionalLightGO->Material = Hazel::CreateRef<Hazel::HMaterial>();
	m_PositionalLightGO->Material->Glossines = 2.0f;
	m_PositionalLightGO->Material->DiffuseTexture = m_Textures[Textures::WhiteTexture];
	m_PositionalLightGO->Material->Color = glm::vec4({ 1.0f, 1.0f, 1.0f, 1.0f });
	m_PositionalLightGO->Transform.SetScale({ 0.3f,0.3, 0.3f });
	m_SceneGO->AddChild(m_PositionalLightGO);

	m_DeferredTexturePass->SetOutput(0, m_Textures[Textures::DeferredTexture]);
	m_DeferredTexturePass->SetInput(0, m_Textures[Textures::DiffuseTexture]);
	m_DeferredTexturePass->SetInput(1, m_Textures[Textures::NormalTexture]);

	m_BaseColorPass->SetInput(0, m_Textures[Textures::DeferredTexture]);
	m_BaseColorPass->SetInput(1, m_Textures[Textures::DiffuseTexture]);
	m_BaseColorPass->SetInput(2, m_Textures[Textures::CubeTexture]);
	m_BaseColorPass->SetInput(3, m_Textures[Textures::WhiteTexture]);
	m_BaseColorPass->SetInput(4, m_Textures[Textures::TriangleTexture]);
	m_BaseColorPass->ClearColor = glm::vec4({ 0.0f, 0.0f, 0.0f, 1.0f });


	m_MipsPass->SetInput(0, m_Textures[Textures::DeferredTexture]);

	m_ClearUAVPass->SetInput(0, m_Textures[Textures::DeferredTexture]);
	m_ClearUAVPass->SetInput(1, m_Textures[Textures::DiffuseTexture]);
	m_ClearUAVPass->SetInput(2, m_Textures[Textures::CubeTexture]);
	m_ClearUAVPass->SetInput(3, m_Textures[Textures::WhiteTexture]);
	m_ClearUAVPass->SetInput(4, m_Textures[Textures::TriangleTexture]);


	Hazel::Application::Get().GetWindow().SetVSync(false);
}

void ExampleLayer::OnAttach()
{
}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate(Hazel::Timestep ts)
{
	HZ_PROFILE_FUNCTION();
	m_CameraController.OnUpdate(ts);

	{
		HZ_PROFILE_SCOPE("Game Object update");
		float angle = ts * 30.0f;
		static glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

		if (m_RotateCube) {
			m_MainObject->Transform.RotateAround(rotationAxis, angle);
		}

		if (use_rendered_texture) {
			m_MainObject->Material->DiffuseTexture = m_Textures[Textures::DeferredTexture];
		}
		else {
			m_MainObject->Material->DiffuseTexture = m_Textures[Textures::DiffuseTexture];
		}
	}

	auto cmdList = m_Context->DeviceResources->CommandList;


	// TODO List:
	// We need a texture cache, this is getting stupid
	// Use Microsoft's DDSLoader to load every texture. At least for the DX12 backend
	// Mipmaps => These will be prepacked into the texture with NVIDIA tools or something.
	// NO we will not generate mipmaps on the fly.
	// PBR Material? Pretty please?
	// Shadow maps
#if 1
	if (m_RenderedFrames >= m_UpdateRate) {
		m_RenderedFrames = 0;

		{
			HZ_PROFILE_SCOPE("Readback update");
			
			auto tex = m_Textures[Textures::DeferredTexture];
			auto feedback = tex->GetFeedbackResource();
			tex->TransitionFeedback(D3D12_RESOURCE_STATE_COPY_SOURCE);
			
			cmdList->CopyResource(m_ReadbackBuffer.Get(), feedback);

			m_Context->Flush();
			tex->TransitionFeedback(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		{
			HZ_PROFILE_SCOPE("Deferred Texture");
			m_DeferredTexturePass->PassData.AmbientLight = m_AmbientLight;
			m_DeferredTexturePass->PassData.AmbientIntensity = m_AmbientIntensity;
			m_DeferredTexturePass->PassData.DirectionalLight = m_PositionalLightGO->Material->Color;
			m_DeferredTexturePass->PassData.DirectionalLightPosition = m_PositionalLightGO->Transform.Position();

			m_DeferredTexturePass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());
		}

		//m_MipsPass->PassData.SourceLevel = 5;

		m_MipsPass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());
	}
#endif
	m_ClearUAVPass->Process(m_Context, m_SceneGO.get(), m_CameraController.GetCamera());

	m_BaseColorPass->Process(m_Context, m_SceneGO.get(), m_CameraController.GetCamera());

#if 1
	if (render_normals) {
		m_NormalsPass->PassData.LightPosition = m_PositionalLightGO->Transform.Position();
		m_NormalsPass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());
	}
#endif
	Hazel::Renderer::EndScene();
	m_RenderedFrames++;
}

void ExampleLayer::OnImGuiRender()
{
	HZ_PROFILE_FUNCTION();
	static bool show_diffuse = false;
	static int diffuse_dim[] = { 512, 512 };

	ImGui::ShowMetricsWindow();
	ImGui::Begin("Controls");
	ImGui::ColorEdit4("Clear Color", &m_BaseColorPass->ClearColor.x);

	auto camera_pos = m_CameraController.GetCamera().GetPosition();
	ImGui::InputFloat3("Camera Position", &camera_pos.x);
	m_CameraController.GetCamera().SetPosition(camera_pos);

	if (ImGui::CollapsingHeader("Main Object")) {

		ImGui::PushID(m_MainObject->Name.c_str());

		ImGui::Text("Transform");
		ImGui::TransformControl(&m_MainObject->Transform);
		ImGui::Text("Material");
		ImGui::MaterialControl(m_MainObject->Material.get());

		ImGui::Text("Mesh");
		ImGui::MeshSelectControl(m_MainObject, m_Models);

		ImGui::Checkbox("Rotate", &m_RotateCube);
		ImGui::Checkbox("Render Normals", &render_normals);
		ImGui::DragFloat("Normal Length", &m_NormalsPass->PassData.NormalLength);
		ImGui::ColorEdit4("Normal Color", &m_NormalsPass->PassData.NormalColor.x);
		ImGui::ColorEdit4("Reflect Color", &m_NormalsPass->PassData.ReflectColor.x);

		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("Secondary Object")) {

		ImGui::PushID(m_SecondaryObject->Name.c_str());

		ImGui::Text("Transform");
		ImGui::TransformControl(&m_SecondaryObject->Transform);
		ImGui::Text("Material");
		ImGui::MaterialControl(m_SecondaryObject->Material.get());

		ImGui::PopID();
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
		ImGui::Checkbox("Show rendered textre", &show_rendered_texture);
		ImGui::Checkbox("Show mip tiles", &show_mip_tiles);
	}


	if (ImGui::CollapsingHeader("Lights")) {
		ImGui::ColorEdit4("Ambient Light Color", &m_AmbientLight.x);
		ImGui::DragFloat("Ambient Light Intensity", &m_AmbientIntensity, 0.1f, 0.0f, 1.0f);

		ImGui::PushID(m_PositionalLightGO->Name.c_str());
		ImGui::Text("Positional Light");
		ImGui::TransformControl(&m_PositionalLightGO->Transform);
		ImGui::MaterialControl(m_PositionalLightGO->Material.get());
		ImGui::PopID();
	}

	ImGui::End();

	ImGui::Begin("Shader Control Center");
	static auto shaderLib = Hazel::ShaderLibrary::GlobalLibrary();
	for (const auto& [key, shader] : *shaderLib)
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

	if (show_rendered_texture) {
		ImGui::Begin("Texture View", &show_rendered_texture);
		ImGui::Image((ImTextureID)m_TextureGPUHandle.ptr, ImVec2(1024, 512));
		ImGui::End();
	}

	if (show_mip_tiles) {
		ImGui::Begin("Mip levels used", &show_mip_tiles);
		auto tex = m_Textures[Textures::DeferredTexture];
		auto dims = tex->GetFeedbackDims();
		void* data;

		m_ReadbackBuffer->Map(0, nullptr, static_cast<void**>(&data));

		const uint32_t* mip_data = reinterpret_cast<uint32_t*>(data);
		uint32_t finest_mip = 13;
		uint32_t coarsest_mip = 0;
		for (int y = 0; y < dims.y; y++)
		{
			for (int x = 0; x < dims.x; x++)
			{
				uint32_t mip = mip_data[y * dims.x + x];

				if (mip < finest_mip) {
					finest_mip = mip;
				}

				if (mip > coarsest_mip) {
					coarsest_mip = mip;
				}
			}
		}

		ImGui::BeginGroup();
		for (int y = 0; y < dims.y; y++)
		{
			ImGui::BeginGroup();
			for (int x = 0; x < dims.x; x++)
			{
				uint32_t mip = mip_data[y * dims.x + x];
				ImGui::PushID(y * dims.x + x);
				if (mip == coarsest_mip) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::ImColor(135, 252, 139));
					ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::ImColor(135, 252, 139));
				}
				else if (mip == finest_mip) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::ImColor(245, 77, 61));
					ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::ImColor(245, 77, 61));
				}

				auto label = std::to_string(mip);
				ImGui::Button(label.c_str(), ImVec2(40, 40));
				ImGui::SameLine();

				if (mip == coarsest_mip || mip == finest_mip) {
					ImGui::PopStyleColor(2);
				}
				ImGui::PopID();
			}
			ImGui::EndGroup();
		}
		ImGui::EndGroup();
		ImGui::Text("Finest Mip: %d, Coarsest Mip: %d", finest_mip, coarsest_mip);
		m_ReadbackBuffer->Unmap(0, nullptr);
		ImGui::End();
	}

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

		m_Textures[Textures::DeferredTexture] = Hazel::CreateRef<Hazel::D3D12Texture2D>(width, height, 13);
		auto tex = m_Textures[Textures::DeferredTexture];

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			1,
			m_Context->GetSRVDescriptorSize()
		);
		tex->GetCommitedResource()->SetName(L"Deferred Texture");

		m_Context->DeviceResources->Device->CreateShaderResourceView(tex->GetCommitedResource(), nullptr, srvHandle);

		m_TextureGPUHandle.InitOffsetted(
			m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
			1,
			m_Context->GetSRVDescriptorSize()
		);

		tex->CreateFeedbackResource(128, 128);

		auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(tex->GetFeedbackSize());

		Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&readbackDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_ReadbackBuffer)
		));

		m_ReadbackBuffer->SetName(L"Readback buffer");
	}

	//Diffuse Texture
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			2,
			m_Context->GetSRVDescriptorSize()
		);

		m_Context->DeviceResources->Device->CreateShaderResourceView(m_Textures[Textures::DiffuseTexture]->GetCommitedResource(), nullptr, srvHandle);
	}
}

void ExampleLayer::LoadAssets()
{
	// Geometry Buffers and Texture Resource
	{
		m_Context->DeviceResources->CommandAllocator->Reset();
		m_Context->DeviceResources->CommandList->Reset(
			m_Context->DeviceResources->CommandAllocator.Get(),
			nullptr
		);


		m_Models[Models::CubeModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_cube.glb"));
		m_Models[Models::SphereModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.glb"));
		m_Models[Models::TriangleModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_triangle.glb"));

		m_Textures[Textures::DiffuseTexture] = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create("assets/textures/earth.dds"));
		m_Textures[Textures::DiffuseTexture]->CreateFeedbackResource(128, 64);
		m_Textures[Textures::DiffuseTexture]->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		/*std::wstring name = L"Diffuse";
		m_DiffuseTexture->DebugNameResource(name);*/

		m_Textures[Textures::NormalTexture] = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create("assets/textures/earth_normal.dds"));
		m_Textures[Textures::NormalTexture]->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		/*name = L"Normal Map";
		m_NormalTexture->DebugNameResource(name);*/

		m_Textures[Textures::WhiteTexture] = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create(1, 1));
		uint8_t white[] = { 255, 255, 255, 255 };
		m_Textures[Textures::WhiteTexture]->Transition(D3D12_RESOURCE_STATE_COPY_DEST);
		m_Textures[Textures::WhiteTexture]->SetData(white, sizeof(white));
		m_Textures[Textures::WhiteTexture]->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		std::wstring name2 = L"White Texture";
		m_Textures[Textures::WhiteTexture]->DebugNameResource(name2);

		m_Textures[Textures::CubeTexture] = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create("assets/textures/Cube_Texture.dds"));
		m_Textures[Textures::CubeTexture]->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		/*name = L"Cube Texture";
		m_CubeTexture->DebugNameResource(name);*/

		m_Textures[Textures::TriangleTexture] = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create("assets/textures/test_texture.dds"));
		m_Textures[Textures::TriangleTexture]->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->CommandList->Close());

		ID3D12CommandList* const commandLists[] = {
			m_Context->DeviceResources->CommandList.Get()
		};
		m_Context->DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		m_Context->Flush();
	}
}

void ExampleLayer::BuildPipeline()
{
	// Color Pass
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

		m_BaseColorPass = Hazel::CreateRef<BaseColorPass>(m_Context, pipelineStateStream);
	}

	// Deferred Pass
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
		pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

		m_DeferredTexturePass = Hazel::CreateRef<DeferredTexturePass>(m_Context, pipelineStateStream);
		m_MipsPass = Hazel::CreateRef<MipMapPass>(m_Context);
	}


	// Normals Pass
	{
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
		rasterizer.FrontCounterClockwise = TRUE;
		rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterizer.DepthClipEnable = FALSE;
		Hazel::D3D12Shader::PipelineStateStream pipelineStateStream;

		pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

		m_NormalsPass = Hazel::CreateRef<NormalsDebugPass>(m_Context, pipelineStateStream);
	}

	// UAV Clear Pass
	{
		m_ClearUAVPass = Hazel::CreateRef<ClearFeedbackPass>(m_Context);
	}
}
