
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
#include "Platform/D3D12/D3D12ResourceBatch.h"

#include"ModelLoader.h"

#include <unordered_map>

#define TEXTURE_WIDTH 4096.0f
#define TEXTURE_HEIGHT 2048.0f
#define USE_VIRTUAL_TEXTURE 1

static bool use_rendered_texture = true;
static bool show_rendered_texture = false;
static bool show_mip_tiles = false;
static bool render_normals = false;
static bool visualize_tiles = false;

static D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static std::pair<uint32_t, uint32_t> extractMipLevels(Hazel::Ref<Hazel::D3D12Texture2D> texture)
{
	auto fb = texture->GetFeedbackMap();
	auto dims = fb->GetDimensions();
	
	uint32_t* data = fb->GetData<uint32_t*>();

	uint32_t finest_mip = 13;
	uint32_t coarsest_mip = 0;
	for (int y = 0; y < dims.y; y++)
	{
		for (int x = 0; x < dims.x; x++)
		{
			uint32_t mip = data[y * dims.x + x];

			if (mip < finest_mip) {
				finest_mip = mip;
			}

			if (mip > coarsest_mip) {
				coarsest_mip = mip;
			}
		}
	}

	return { finest_mip, coarsest_mip };
}

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
	m_TilePool = Hazel::CreateRef<Hazel::D3D12TilePool>();

	m_Models.resize(Models::CountModel);
	m_Textures.resize(Textures::CountTexure);

	LoadAssets();
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
#if USE_VIRTUAL_TEXTURE
	m_MainObject->Material->AlbedoTexture = m_Textures[Textures::VirtualTexture];
#else
	m_MainObject->Material->DiffuseTexture = m_Textures[Textures::DeferredTexture];
#endif
	m_MainObject->Material->NormalTexture = m_Textures[Textures::NormalTexture];
	m_MainObject->Material->Color = glm::vec4(1.0f);
	m_SceneGO->AddChild(m_MainObject);
	// Cube #2
	m_SecondaryObject = Hazel::CreateRef<Hazel::GameObject>();
	m_SecondaryObject->Name = "Cube#2";
	m_SecondaryObject->Transform.SetPosition(glm::vec3(0.0f, 1.0f, 0.0f));
	m_SecondaryObject->Mesh = m_Models[Models::TriangleModel]->Mesh;
	m_SecondaryObject->Material = Hazel::CreateRef<Hazel::HMaterial>();
	m_SecondaryObject->Material->Glossines = 2.0f;
	m_SecondaryObject->Material->AlbedoTexture = m_Textures[Textures::TriangleTexture];
	m_SecondaryObject->Material->Color = glm::vec4(1.0f);
	m_MainObject->AddChild(m_SecondaryObject);

	// "Light" Sphere
	m_PositionalLightGO = Hazel::CreateRef<Hazel::GameObject>();
	m_PositionalLightGO->Name = "Light Sphere";
	m_PositionalLightGO->Mesh = m_Models[Models::SphereModel]->Mesh;
	m_PositionalLightGO->Transform.SetPosition({ 1.0, 8.0, 8.0 });
	m_PositionalLightGO->Material = Hazel::CreateRef<Hazel::HMaterial>();
	m_PositionalLightGO->Material->Glossines = 2.0f;
	m_PositionalLightGO->Material->AlbedoTexture = m_Textures[Textures::WhiteTexture];
	m_PositionalLightGO->Material->Color = glm::vec4({ 1.0f, 1.0f, 1.0f, 1.0f });
	m_PositionalLightGO->Transform.SetScale({ 0.3f,0.3, 0.3f });
	m_SceneGO->AddChild(m_PositionalLightGO);

#if USE_VIRTUAL_TEXTURE
	m_DeferredTexturePass->SetOutput(0, m_Textures[Textures::VirtualTexture]);
#else
	m_DeferredTexturePass->SetOutput(0, m_Textures[Textures::DeferredTexture]);
#endif
	m_DeferredTexturePass->SetInput(0, m_Textures[Textures::EarthTexture]);
	m_DeferredTexturePass->SetInput(1, m_Textures[Textures::NormalTexture]);

#if USE_VIRTUAL_TEXTURE
	m_BaseColorPass->SetInput(0, m_Textures[Textures::VirtualTexture]);
#else
	m_BaseColorPass->SetInput(0, m_Textures[Textures::DeferredTexture]);
#endif
	m_BaseColorPass->SetInput(1, m_Textures[Textures::EarthTexture]);
	m_BaseColorPass->SetInput(2, m_Textures[Textures::CubeTexture]);
	m_BaseColorPass->SetInput(3, m_Textures[Textures::WhiteTexture]);
	m_BaseColorPass->SetInput(4, m_Textures[Textures::TriangleTexture]);
	m_BaseColorPass->ClearColor = glm::vec4({ 0.0f, 0.0f, 0.0f, 1.0f });

#if USE_VIRTUAL_TEXTURE
	m_MipsPass->SetInput(0, m_Textures[Textures::VirtualTexture]);
#else
	m_MipsPass->SetInput(0, m_Textures[Textures::DeferredTexture]);
#endif

#if USE_VIRTUAL_TEXTURE
	m_ClearUAVPass->SetInput(0, m_Textures[Textures::VirtualTexture]);
#else
	m_ClearUAVPass->SetInput(0, m_Textures[Textures::DeferredTexture]);
#endif
	m_ClearUAVPass->SetInput(1, m_Textures[Textures::EarthTexture]);
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
	//HZ_CORE_INFO("New Frame");

	{
		HZ_PROFILE_SCOPE("Game Object update");
		float angle = ts * 30.0f;
		static glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

		if (m_RotateCube) {
			m_MainObject->Transform.RotateAround(rotationAxis, angle);
		}

		if (use_rendered_texture) {
#if USE_VIRTUAL_TEXTURE
			m_MainObject->Material->AlbedoTexture = m_Textures[Textures::VirtualTexture];
#else
			m_MainObject->Material->DiffuseTexture = m_Textures[Textures::DeferredTexture];
#endif
		}
		else {
			m_MainObject->Material->AlbedoTexture = m_Textures[Textures::EarthTexture];
		}
	}

	auto cmdList = m_Context->DeviceResources->CommandList;
	auto device = m_Context->DeviceResources->Device;

#if 1
	if (m_RenderedFrames >= m_UpdateRate) {
		m_RenderedFrames = 0;
		//HZ_CORE_INFO("New Deferred Frame");

		auto tex = m_MainObject->Material->AlbedoTexture;
		{
			HZ_PROFILE_SCOPE("Readback update");
			
			auto feedback = tex->GetFeedbackMap();
			Hazel::D3D12ResourceBatch batch(device);
			auto cmd = batch.Begin();
			feedback->Readback(device.Get(), cmd.Get());
			batch.End(m_Context->DeviceResources->CommandQueue.Get()).wait();
		}

		auto mips = tex->ExtractMipsUsed();
		uint32_t fine = mips.FinestMip;

		if (fine >= tex->GetMipLevels())
		{
			fine = tex->GetMipLevels() - 1;
		}

#if USE_VIRTUAL_TEXTURE
		{
			Hazel::D3D12ResourceBatch batch(device);
			auto cmd = batch.Begin();
			m_TilePool->MapTexture(batch, tex, m_Context->DeviceResources->CommandQueue);
			batch.End(m_Context->DeviceResources->CommandQueue.Get()).wait();
		}
#endif

		{
			HZ_PROFILE_SCOPE("Deferred Texture");
			m_DeferredTexturePass->PassData.AmbientLight = m_AmbientLight;
			m_DeferredTexturePass->PassData.AmbientIntensity = m_AmbientIntensity;
			m_DeferredTexturePass->PassData.DirectionalLight = m_PositionalLightGO->Material->Color;
			m_DeferredTexturePass->PassData.DirectionalLightPosition = m_PositionalLightGO->Transform.Position();
			m_DeferredTexturePass->PassData.FinestMip = fine;
			m_DeferredTexturePass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());
		}

		m_MipsPass->PassData.SourceLevel = fine;

		m_MipsPass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());
	}
#endif
	m_ClearUAVPass->Process(m_Context, m_SceneGO.get(), m_CameraController.GetCamera());

	m_BaseColorPass->Process(m_Context, m_SceneGO.get(), m_CameraController.GetCamera());

	Hazel::Renderer::EndScene();
	m_RenderedFrames++;
}

void ExampleLayer::OnImGuiRender()
{
	HZ_PROFILE_FUNCTION();
	static bool show_diffuse = false;
	static int diffuse_dim[] = { 512, 512 };

	ImGui::ShowMetricsWindow();
#if 1
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
		//ImGui::Checkbox("Show rendered textre", &show_rendered_texture);
		ImGui::Checkbox("Show mip tiles", &show_mip_tiles);
		//ImGui::Checkbox("Show tile visualization", &visualize_tiles);
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

	if (show_mip_tiles) {
		ImGui::Begin("Mip levels used", &show_mip_tiles);

		auto tex = m_MainObject->Material->AlbedoTexture;

		auto mipsUsed = tex->GetMipsUsed();
		auto feedback = tex->GetFeedbackMap();
		auto dims = feedback->GetDimensions();
		
		const uint32_t* mip_data = feedback->GetData<uint32_t*>();

		ImGui::BeginGroup();
		for (int y = 0; y < dims.y; y++)
		{
			ImGui::BeginGroup();
			for (int x = 0; x < dims.x; x++)
			{
				uint32_t mip = mip_data[y * dims.x + x];
				ImGui::PushID(y * dims.x + x);
				if (mip == mipsUsed.CoarsestMip) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::ImColor(135, 252, 139));
					ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::ImColor(0, 0, 0));
				}
				else if (mip == mipsUsed.FinestMip) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::ImColor(245, 77, 61));
					ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::ImColor(255, 255, 255));
				}

				auto index = y * dims.x + x;

				auto label = std::to_string(mip);
				ImGui::Button(label.c_str(), ImVec2(40, 40));
				ImGui::SameLine();

				if (mip == mipsUsed.CoarsestMip || mip == mipsUsed.FinestMip) {
					ImGui::PopStyleColor(2);
				}
				ImGui::PopID();
			}
			ImGui::EndGroup();
		}
		ImGui::EndGroup();
		ImGui::Text("Finest Mip: %d, Coarsest Mip: %d", mipsUsed.FinestMip, mipsUsed.CoarsestMip);
		ImGui::End();
	}

#endif
}

void ExampleLayer::OnEvent(Hazel::Event& e)
{
	m_CameraController.OnEvent(e);
}

void ExampleLayer::LoadAssets()
{
	// Geometry Buffers and Texture Resource
	{
		Hazel::D3D12ResourceBatch batch(m_Context->DeviceResources->Device);
		auto cmdlist = batch.Begin();

		m_Models[Models::CubeModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_cube.glb"), batch);
		m_Models[Models::SphereModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.glb"), batch);
		m_Models[Models::TriangleModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_triangle.glb"), batch);
		
		{
			HZ_PROFILE_SCOPE("Deferred Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Name = L"Deferred Texture";
			opts.Width = TEXTURE_WIDTH;
			opts.Height = TEXTURE_HEIGHT;
			opts.MipLevels = 13;
			opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
				| D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
				| D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

			m_Textures[Textures::DeferredTexture] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			auto tex = m_Textures[Textures::DeferredTexture];
			auto fb = Hazel::D3D12Texture2D::CreateFeedbackMap(batch, tex);
			tex->SetFeedbackMap(fb);

			// We need the following handles so ImGui can show this texture
			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
				m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
				1,
				m_Context->GetSRVDescriptorSize()
			);

			m_Context->DeviceResources
				->Device
				->CreateShaderResourceView(tex->GetResource(), nullptr, srvHandle);

			m_TextureGPUHandle.InitOffsetted(
				m_Context->DeviceResources->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
				1,
				m_Context->GetSRVDescriptorSize()
			);
		}
		
		{
			HZ_PROFILE_SCOPE("Earth Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			opts.Path = L"assets/textures/earth.dds";
			m_Textures[Textures::EarthTexture] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			m_Textures[Textures::EarthTexture]->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			
			// We need this handle so ImGui can display this
			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
				m_Context->DeviceResources->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
				2,
				m_Context->GetSRVDescriptorSize()
			);

			m_Context->DeviceResources->Device->CreateShaderResourceView(m_Textures[Textures::EarthTexture]->GetResource(), nullptr, srvHandle);
		}

		{
			HZ_PROFILE_SCOPE("Virtual Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Name = L"Earth Virtual Texture";
			opts.Width = TEXTURE_WIDTH;
			opts.Height = TEXTURE_HEIGHT;
			opts.MipLevels = 13;
			opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
				| D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			auto tex = m_Textures[Textures::VirtualTexture] = Hazel::D3D12Texture2D::CreateVirtualTexture(batch, opts);
			auto fb = Hazel::D3D12Texture2D::CreateFeedbackMap(batch, tex);
			tex->SetFeedbackMap(fb);
		}
		
		{
			HZ_PROFILE_SCOPE("Normal Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			opts.Path = L"assets/textures/earth_normal.dds";
			m_Textures[Textures::NormalTexture] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			m_Textures[Textures::NormalTexture]->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		{
			HZ_PROFILE_SCOPE("White Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Name = L"White Texture";
			opts.Width = 1;
			opts.Height = 1;
			opts.MipLevels = 1;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;

			uint8_t white[] = { 255, 255, 255, 255 };
			m_Textures[Textures::WhiteTexture] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			m_Textures[Textures::WhiteTexture]->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
			m_Textures[Textures::WhiteTexture]->SetData(batch, white, sizeof(white));
			m_Textures[Textures::WhiteTexture]->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		{
			HZ_PROFILE_SCOPE("Cube Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			opts.Path = L"assets/textures/Cube_Texture.dds";

			m_Textures[Textures::CubeTexture] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			m_Textures[Textures::CubeTexture]->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		{
			HZ_PROFILE_SCOPE("Triangle Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			opts.Path = L"assets/textures/test_texture.dds";
			m_Textures[Textures::TriangleTexture] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			m_Textures[Textures::TriangleTexture]->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		{
			HZ_PROFILE_SCOPE("Mip Debug Texture");
			Hazel::D3D12Texture2D::TextureCreationOptions opts;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			opts.Path = L"assets/textures/mips_debug.dds";
			m_Textures[Textures::MipDebug] = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
			m_Textures[Textures::MipDebug]->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
		
		auto f = batch.End(m_Context->DeviceResources->CommandQueue.Get());
		f.wait();
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
