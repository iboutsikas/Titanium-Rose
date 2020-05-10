
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
	m_AmbientIntensity(1.0f),
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
	m_MainObject->Mesh = m_Models[Models::TriangleModel]->Mesh;
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
	//m_DeferredTexturePass->SetInput(0, m_DiffuseTexture);
	m_DeferredTexturePass->SetInput(0, m_Textures[Textures::TriangleTexture]);
	m_DeferredTexturePass->SetInput(1, m_Textures[Textures::NormalTexture]);

	m_BaseColorPass->SetInput(0, m_Textures[Textures::DeferredTexture]);
	m_BaseColorPass->SetInput(1, m_Textures[Textures::DiffuseTexture]);
	m_BaseColorPass->SetInput(2, m_Textures[Textures::CubeTexture]);
	m_BaseColorPass->SetInput(3, m_Textures[Textures::WhiteTexture]);
	m_BaseColorPass->SetInput(4, m_Textures[Textures::TriangleTexture]);
	m_BaseColorPass->ClearColor = glm::vec4({ 0.0f, 0.0f, 0.0f, 1.0f });


	m_MipsPass->SetInput(0, m_Textures[Textures::DeferredTexture]);

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

	auto rot_matrix = m_MainObject->Transform.RotationMatrix();
	auto transform = m_MainObject->Transform.LocalToWorldMatrix();


	//auto cmdList = m_Context->DeviceResources->CommandList;


	// TODO List:
	// We need a texture cache, this is getting stupid
	// Use Microsoft's DDSLoader to load every texture. At least for the DX12 backend
	// Mipmaps => These will be prepacked into the texture with NVIDIA tools or something.
	// NO we will not generate mipmaps on the fly.
	// PBR Material? Pretty please?
	// Shadow maps

#if 1
	if (m_RenderedFrames >= m_UpdateRate) {
		HZ_PROFILE_SCOPE("Deferred Texture");
		m_RenderedFrames = 0;

		glm::mat4 m = m_SecondaryObject->Transform.WorldToLocalMatrix();
		glm::mat4 vp = m_CameraController.GetCamera().GetViewProjectionMatrix();
		glm::mat4 mvp = vp * m;

		glm::vec3 pos = m_SecondaryObject->Transform.Position();
		glm::vec3 tan = m_SecondaryObject->Mesh->maxTangent;
		glm::vec3 bitan = m_SecondaryObject->Mesh->maxBitangent;

		//glm::vec4 vec1 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		//glm::vec4 vec2 = mvp * vec1;
		//vec2 /= vec2.w;

		glm::mat3 jacobian_matrix = glm::mat3(0.0f);

		// This is also the same as mvp * vec4(pos, 1.0) but for now let's leave it like that
		float nomX  = (mvp[0][0] * pos.x) + (mvp[1][0] * pos.y) + (mvp[2][0] * pos.z) + mvp[3][0];
		float nomY  = (mvp[0][1] * pos.x) + (mvp[1][1] * pos.y) + (mvp[2][1] * pos.z) + mvp[3][1];
		float nomZ  = (mvp[0][2] * pos.x) + (mvp[1][2] * pos.y) + (mvp[2][2] * pos.z) + mvp[3][2];
		float denom = (mvp[0][3] * pos.x) + (mvp[1][3] * pos.y) + (mvp[2][3] * pos.z) + mvp[3][3];

		// Can also precal nomXYZ / denom**2

		// diff(T, x)
		jacobian_matrix[0][0] = (mvp[0][0] / denom) - ((mvp[0][3] * nomX) / (denom * denom));
		jacobian_matrix[0][1] = (mvp[0][1] / denom) - ((mvp[0][3] * nomY) / (denom * denom));
		jacobian_matrix[0][2] = (mvp[0][2] / denom) - ((mvp[0][3] * nomZ) / (denom * denom));

		// diff(T, y)
		jacobian_matrix[1][0] = (mvp[1][0] / denom) - ((mvp[1][3] * nomX) / (denom * denom));
		jacobian_matrix[1][1] = (mvp[1][1] / denom) - ((mvp[1][3] * nomY) / (denom * denom));
		jacobian_matrix[1][2] = (mvp[1][2] / denom) - ((mvp[1][3] * nomZ) / (denom * denom));
		
		// diff(T, z)
		jacobian_matrix[2][0] = (mvp[2][0] / denom) - ((mvp[2][3] * nomX) / (denom * denom));
		jacobian_matrix[2][1] = (mvp[2][1] / denom) - ((mvp[2][3] * nomY) / (denom * denom));
		jacobian_matrix[2][2] = (mvp[2][2] / denom) - ((mvp[2][3] * nomZ) / (denom * denom));


		float dXdu = tan.x * jacobian_matrix[0][0] + tan.y * jacobian_matrix[1][0] + tan.z * jacobian_matrix[2][0];
		float dYdu = tan.x * jacobian_matrix[0][1] + tan.y * jacobian_matrix[1][1] + tan.z * jacobian_matrix[2][1];
		
		float dXdv = bitan.x * jacobian_matrix[0][0] + bitan.y * jacobian_matrix[1][0] + bitan.z * jacobian_matrix[2][0];
		float dYdv = bitan.x * jacobian_matrix[0][1] + bitan.y * jacobian_matrix[1][1] + bitan.z * jacobian_matrix[2][1];
		
		glm::vec2 du = glm::vec2(0.0f);
		du.x = dXdu != 0.0f ? 1.0f / dXdu : 0.0f;
		du.y = dYdu != 0.0f ? 1.0f / dYdu : 0.0f;
		
		glm::vec2 dv = glm::vec2(0.0f); 
		dv.x = dXdv != 0.0f ? 1.0f / dXdv : 0.0f;
		dv.y = dYdv != 0.0f ? 1.0f / dYdv : 0.0f;

		float lambda = 0.0f;
		float lambda_prime = 0.0f;
		// This is the level of detail calculcation based on the opengl spec
		{
			float lod_min = 0.0f;
			float lod_max = 1000.0f;
#if 0 // Calculations involving anisotropy from OpenGL 4.6
			auto px = std::sqrtf(du.x * du.x + dv.x * dv.x);
			auto py = std::sqrtf(du.y * du.y + dv.y * dv.y);


			auto p_max = std::max(px, py);
			auto p_min = std::min(px, py);

			auto n = std::min(std::ceil(p_max / p_min), 1.0f);
			auto p = p_max / n;
			lambda_prime = std::log2f(p);
#endif			
#if 1 // Calculations without anisotropy from OpenGL 4.5			
			auto rho = std::max(glm::length(du), glm::length(dv));
			auto lambda_base = (std::log(rho) / std::log(2.0));
			lambda_prime = lambda_base + 0.0f; // Zero bias for now /*std::log2f(p);*/
#endif
			if (lambda_prime > lod_max) 
			{
				lambda = lod_max;
			}
			else if (lod_min <= lambda_prime <= lod_max)
			{
				lambda = lambda_prime;
			}
			else if (lambda_prime < lod_min)
			{
				lambda = lod_min;
			}
		}
		// Selecting mipmap based on NEAREST_MIPMAP_NEAREST and LINEAR_MIPMAP_NEAREST, on OpenGL spec
		float d = 0.0f;
		{
			float level_base = 0;
			float level_max = m_Textures[Textures::DeferredTexture]->GetMipLevels() - 1;
			// This is the same level max. The spec expresses it that way cuz the texture might be
			// Some other subresource than the original texture. For us they are the same for now
			float p = std::floor(std::log2(TEXTURE_WIDTH)) + level_base; 
			float q = std::min(p, level_max);
						
			if (lambda <= 0.0f) 
			{
				d = level_base;
			}
			else {
				if (level_base + lambda <= q + 0.5f) {
					d = (level_base + lambda + 0.5f) - 1.0f;
				}
				else {
					d = q;
				}
			}
		}

#if 0
		auto tangent4 = glm::vec4(
			m_MainObject->Mesh->maxTangent.x,
			m_MainObject->Mesh->maxTangent.y,
			m_MainObject->Mesh->maxTangent.z,
			1.0f);

		auto bitangent4 = glm::vec4(
			m_MainObject->Mesh->maxBitangent.x,
			m_MainObject->Mesh->maxBitangent.y,
			m_MainObject->Mesh->maxBitangent.z,
			1.0f);

		auto worldTangent = mvp * tangent4;
		auto worldBitangent = mvp * bitangent4;

		auto dTangent = glm::length(m_MainObject->Mesh->maxTangent) / glm::length(worldTangent);
		auto dBitangent = glm::length(m_MainObject->Mesh->maxBitangent) / glm::length(worldBitangent);
		auto dMax = std::max(dTangent, dBitangent);

		auto maxMip = -std::log2f(dMax);

#endif
		m_BlendFactor = std::modff(d, &m_MaxMip);

		m_DeferredTexturePass->PassData.AmbientLight = m_AmbientLight;
		m_DeferredTexturePass->PassData.AmbientIntensity = m_AmbientIntensity;
		m_DeferredTexturePass->PassData.DirectionalLight = m_PositionalLightGO->Material->Color;
		m_DeferredTexturePass->PassData.DirectionalLightPosition = m_PositionalLightGO->Transform.Position();

		m_DeferredTexturePass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());

		m_MipsPass->Process(m_Context, m_MainObject.get(), m_CameraController.GetCamera());
	}
#endif

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
		ImGui::Text("Max Mip: %.0f, Blend factor: %.6f", m_MaxMip, m_BlendFactor);
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

		m_Textures[Textures::DiffuseTexture] = std::dynamic_pointer_cast<Hazel::D3D12Texture2D>(Hazel::Texture2D::Create("assets/textures/mips_debug.dds"));
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
}
