#include "hzpch.h"
#include "D3D12Renderer.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12ForwardRenderer.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "glm/gtc/type_ptr.hpp"

#include "WinPixEventRuntime/pix3.h"

#define MAX_RESOURCES       100
#define MAX_RENDERTARGETS   50

struct SkyboxVertex {
	glm::vec3 Position;
};

namespace Hazel {
	D3D12Context*	D3D12Renderer::Context;
	ShaderLibrary*	D3D12Renderer::ShaderLibrary;
	TextureLibrary* D3D12Renderer::TextureLibrary;

	D3D12DescriptorHeap* D3D12Renderer::s_ResourceDescriptorHeap;
	D3D12DescriptorHeap* D3D12Renderer::s_RenderTargetDescriptorHeap;
	D3D12VertexBuffer* D3D12Renderer::s_SkyboxVB;
	D3D12IndexBuffer* D3D12Renderer::s_SkyboxIB;

	D3D12Renderer::CommonData D3D12Renderer::s_CommonData;

	Scope<D3D12UploadBuffer<D3D12Renderer::RendererLight>> D3D12Renderer::s_LightsBuffer;
	HeapAllocationDescription D3D12Renderer::s_LightsBufferAllocation;



	D3D12_INPUT_ELEMENT_DESC D3D12Renderer::s_InputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Binormal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Hazel::Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	uint32_t D3D12Renderer::s_InputLayoutCount = _countof(s_InputLayout);

	std::vector<Ref<GameObject>> D3D12Renderer::s_OpaqueObjects;
	std::vector<Ref<GameObject>> D3D12Renderer::s_TransparentObjects;
	std::vector<D3D12Renderer*> D3D12Renderer::s_AvailableRenderers;
	D3D12Renderer* D3D12Renderer::s_CurrentRenderer = nullptr;

	void D3D12Renderer::PrepareBackBuffer(glm::vec4 clear)
	{
		D3D12ResourceBatch batch(Context->DeviceResources->Device);
		auto cmdList = batch.Begin();
		auto backBuffer = Context->GetCurrentBackBuffer();

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAG_NONE);

		cmdList->ResourceBarrier(1, &barrier);

		cmdList->ClearRenderTargetView(Context->CurrentBackBufferView(), &clear.x, 0, nullptr);
		cmdList->ClearDepthStencilView(Context->DepthStencilView(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			1.0f, 0, 0, nullptr
		);

		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
	}

	void D3D12Renderer::NewFrame()
	{
		Context->NewFrame();

		ShaderLibrary->Update();

		if (s_CommonData.DynamicResources != 0)
		{
			HeapAllocationDescription desc;
			desc.Allocated = true;
			desc.OffsetInHeap = s_CommonData.StaticResources;
			desc.Range = s_CommonData.DynamicResources;

			s_ResourceDescriptorHeap->Release(desc);
			HZ_CORE_ASSERT(desc.Allocated == false, "Could not release dynamic resources");

			s_CommonData.DynamicResources = 0;
		}

		if (s_CommonData.DynamicRenderTargets != 0)
		{
			HeapAllocationDescription desc;
			desc.Allocated = true;
			desc.OffsetInHeap = s_CommonData.StaticRenderTargets;
			desc.Range = s_CommonData.DynamicRenderTargets;

			s_RenderTargetDescriptorHeap->Release(desc);
			HZ_CORE_ASSERT(desc.Allocated == false, "Could not release dynamic render targets");

			s_CommonData.DynamicRenderTargets == 0;
		}
	}

	void D3D12Renderer::Present()
	{
		Context->SwapBuffers();
	}

	void D3D12Renderer::BeginScene(Scene& scene)
	{
		s_CommonData.Scene = &scene;

		for (size_t i = 0; i < scene.Lights.size(); i++)
		{
			auto& l = scene.Lights[i];

			RendererLight rl;
			rl.Color = l.GameObject->Material->Color;
			rl.Position = glm::vec4(l.GameObject->Transform.Position(), 1.0f);
			rl.Range = l.Range;
			rl.Intensity = l.Intensity;
			s_LightsBuffer->CopyData(i, rl);

			s_CommonData.NumLights++;
		}
	}

	void D3D12Renderer::EndScene()
	{
		s_CommonData.NumLights = 0;
		s_CommonData.Scene = nullptr;
	}

	void D3D12Renderer::ResizeViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		Context->m_Viewport.TopLeftX = x;
		Context->m_Viewport.TopLeftY = y;
		Context->m_Viewport.Width = width;
		Context->m_Viewport.Height = height;
		Context->m_Viewport.MinDepth = 0.0f;
		Context->m_Viewport.MaxDepth = 1.0f;

		Context->m_ScissorRect.left = x;
		Context->m_ScissorRect.top = y;
		Context->m_ScissorRect.right = width;
		Context->m_ScissorRect.bottom = height;

		Context->Flush();
		auto r = Context->DeviceResources.get();
		Context->CleanupRenderTargetViews();
		Context->ResizeSwapChain();
		Context->CreateRenderTargetViews();
		Context->CreateDepthStencil();

		// Transition the DepthStencilBuffer
		auto dsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			r->DepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);

		r->CommandList->ResourceBarrier(1, &dsBarrier);

		// Execute all the resize magic
		D3D12::ThrowIfFailed(r->CommandList->Close());

		ID3D12CommandList* const commandLists[] = {
			r->CommandList.Get()
		};
		r->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	}

	void D3D12Renderer::SetVCsync(bool enable)
	{
		Context->SetVSync(enable);
	}

	void D3D12Renderer::AddStaticResource(Ref<D3D12Texture> texture)
	{
		if (s_CommonData.DynamicResources != 0) {
			HZ_CORE_WARN("Tried to allocate static resource after a dynamic resource");
			return;
		}

		texture->DescriptorAllocation = s_ResourceDescriptorHeap->Allocate(1);
		HZ_CORE_ASSERT(texture->DescriptorAllocation.Allocated, "Could not allocate space on the resource heap");
		D3D12Renderer::Context->DeviceResources->Device->CreateShaderResourceView(
			texture->GetResource(),
			nullptr,
			texture->DescriptorAllocation.CPUHandle
		);
		s_CommonData.StaticResources++;
	}

	void D3D12Renderer::AddDynamicResource(Ref<D3D12Texture> texture)
	{

		texture->DescriptorAllocation = s_ResourceDescriptorHeap->Allocate(1);
		HZ_CORE_ASSERT(texture->DescriptorAllocation.Allocated, "Could not allocate space on the resource heap");
		
		auto desc = texture->GetResource()->GetDesc();

		auto dim = texture->IsCube() ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = dim;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		switch (dim)
		{
		case D3D12_SRV_DIMENSION_TEXTURE2D: {
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = texture->GetMipLevels();
		}
		case D3D12_SRV_DIMENSION_TEXTURECUBE: {
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = texture->GetMipLevels();
		}
		}

		D3D12Renderer::Context->DeviceResources->Device->CreateShaderResourceView(
			texture->GetResource(),
			&srvDesc,
			texture->DescriptorAllocation.CPUHandle
		);
		s_CommonData.DynamicResources++;
	}

	void D3D12Renderer::AddStaticRenderTarget(Ref<D3D12Texture> texture)
	{
	}

	void D3D12Renderer::Submit(Hazel::Ref<Hazel::GameObject>& go)
	{
		if (go->Mesh != nullptr)
		{
			if (go->Material->IsTransparent)
			{
				s_TransparentObjects.push_back(go);
			}
			else
			{
				s_OpaqueObjects.push_back(go);
			}
		}

		for (auto& c : go->children)
		{
			Submit(c);
		}
	}

	void D3D12Renderer::RenderSubmitted()
	{
		s_CurrentRenderer->ImplRenderSubmitted();
		s_TransparentObjects.clear();
		s_OpaqueObjects.clear();
	}

	void D3D12Renderer::RenderSkybox()
	{
		auto shader = ShaderLibrary->GetAs<D3D12Shader>("skybox");
		
		auto camera = s_CommonData.Scene->Camera;
		struct {
			glm::mat4 ViewInverse;
			glm::mat4 ProjectionTranspose;
		} PassData;
		PassData.ViewInverse = glm::inverse(camera->GetViewMatrix());
		PassData.ProjectionTranspose = glm::inverse(camera->GetProjectionMatrix());

		auto& env = s_CommonData.Scene->Environment;

		auto vb = s_SkyboxVB->GetView();
		vb.StrideInBytes = sizeof(SkyboxVertex);
		auto ib = s_SkyboxIB->GetView();


		D3D12ResourceBatch batch(Context->DeviceResources->Device);
		auto cmdList = batch.Begin();
		PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Skybox pass");

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &vb);
		cmdList->IASetIndexBuffer(&ib);
		cmdList->OMSetRenderTargets(1, &Context->CurrentBackBufferView(), TRUE, &Context->DepthStencilView());
		cmdList->RSSetViewports(1, &Context->m_Viewport);
		cmdList->RSSetScissorRects(1, &Context->m_ScissorRect);
		ID3D12DescriptorHeap* descriptorHeaps[] = {
			s_ResourceDescriptorHeap->GetHeap()
		};
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
		cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
		cmdList->SetPipelineState(shader->GetPipelineState());
		cmdList->SetGraphicsRoot32BitConstants(0, sizeof(PassData) / sizeof(float), &PassData, 0);
		cmdList->SetGraphicsRootDescriptorTable(1, env.EnvironmentMap->DescriptorAllocation.GPUHandle);

		cmdList->DrawIndexedInstanced(s_SkyboxIB->GetCount(), 1, 0, 0, 0);

		PIXEndEvent(cmdList.Get());
		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
	}

	std::pair<Ref<D3D12TextureCube>, Ref<D3D12TextureCube>> D3D12Renderer::LoadEnvironmentMap(std::string& path)
	{
		if (TextureLibrary->Exists(path))
		{
			auto tex = TextureLibrary->GetAs<D3D12TextureCube>(path);

			return { tex, nullptr };
		}

		std::string extension = path.substr(path.find_last_of(".") + 1);

		if (extension == "hdri")
		{
			// Load from hdri
		}
		else if (extension == "dds")
		{

			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			batch.Begin();

			D3D12Texture::TextureCreationOptions opts;
			opts.Path = path;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			auto tex = D3D12TextureCube::Create(batch, opts);

			batch.End(Context->DeviceResources->CommandQueue.Get());

			TextureLibrary->Add(tex);

			return { tex, nullptr };
		}
		else
		{
			HZ_CORE_ASSERT(false, "Image file is not supported");
		}

		return std::pair<Ref<D3D12TextureCube>, Ref<D3D12TextureCube>>();
	}

	void D3D12Renderer::Init()
	{
		auto& wnd = Application::Get().GetWindow();
		Context = new D3D12Context();
		Context->Init(&wnd);

		D3D12Renderer::ShaderLibrary = new Hazel::ShaderLibrary(3);
		D3D12Renderer::TextureLibrary = new Hazel::TextureLibrary();

#pragma region Renderer Implementations
		// Add the two renderer implementations
		{
			s_AvailableRenderers.resize(RendererType_Count);
			s_AvailableRenderers[RendererType_Forward] = new D3D12ForwardRenderer();
			s_AvailableRenderers[RendererType_Forward]->ImplOnInit();
			// s_AvailableRenderers[RendererType_TextureSpace] = new Texture space renderer
			s_CurrentRenderer = s_AvailableRenderers[RendererType_Forward];
		}
#pragma endregion

#pragma region Heaps		
		// Create the resource heaps for all the renderer allocations
		{
			s_ResourceDescriptorHeap = new D3D12DescriptorHeap(
				D3D12Renderer::Context->DeviceResources->Device.Get(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				MAX_RESOURCES
			);

			s_RenderTargetDescriptorHeap = new D3D12DescriptorHeap(
				D3D12Renderer::Context->DeviceResources->Device.Get(),
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				MAX_RENDERTARGETS
			);
		}
#pragma endregion

#pragma region Scene Lights Buffer
		// Create the buffer for the scene lights
		{
			HZ_CORE_ASSERT((sizeof(RendererLight) % 16) == 0, "The size of the light struct should be 128-bit aligned");

			s_LightsBuffer = CreateScope<D3D12UploadBuffer<RendererLight>>(MaxSupportedLights, false);
			s_LightsBuffer->Resource()->SetName(L"Lights buffer");

			s_LightsBufferAllocation = s_ResourceDescriptorHeap->Allocate(1);
			HZ_CORE_ASSERT(s_LightsBufferAllocation.Allocated, "Could not allocate the light buffer");
			s_CommonData.StaticResources++;

			D3D12_SHADER_RESOURCE_VIEW_DESC desc;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = MaxSupportedLights;
			desc.Buffer.StructureByteStride = sizeof(RendererLight);
			desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			Context->DeviceResources->Device->CreateShaderResourceView(
				s_LightsBuffer->Resource(),
				&desc,
				s_LightsBufferAllocation.CPUHandle
			);
		}
#pragma endregion
		
#pragma region Engine Shaders
		// Skybox
		{
			static D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			D3D12_RT_FORMAT_ARRAY rtvFormats = {};
			rtvFormats.NumRenderTargets = 1;
			rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
			rasterizer.FrontCounterClockwise = TRUE;

			CD3DX12_PIPELINE_STATE_STREAM pipelineStateStream;

			pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
			pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			pipelineStateStream.RTVFormats = rtvFormats;
			pipelineStateStream.RasterizerState = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);
			
			CD3DX12_DEPTH_STENCIL_DESC1 depthDesc(D3D12_DEFAULT);
			depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

			pipelineStateStream.DepthStencilState = CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1(depthDesc);

			auto shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/skybox.hlsl", pipelineStateStream);
			D3D12Renderer::ShaderLibrary->Add(shader);

			std::vector<SkyboxVertex> verts(4);
			verts[0] = { { -1.0f , -1.0f, 0.1f } };
			verts[1] = { {  1.0f , -1.0f, 0.1f } };
			verts[2] = { {  1.0f ,  1.0f, 0.1f } };
			verts[3] = { { -1.0f ,  1.0f, 0.1f } };

			std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			batch.Begin();

			s_SkyboxVB = new D3D12VertexBuffer(batch, (float*)verts.data(), verts.size() * sizeof(SkyboxVertex));
			s_SkyboxIB = new D3D12IndexBuffer(batch, indices.data(), indices.size());

			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
		}
#pragma endregion


	}

	void D3D12Renderer::Shutdown()
	{
		s_LightsBuffer.release();
		delete s_SkyboxVB;
		delete s_SkyboxIB;

		delete Context;
		delete ShaderLibrary;
		delete TextureLibrary;

		delete s_ResourceDescriptorHeap;
		delete s_RenderTargetDescriptorHeap;

		// TODO: Flush pipeline maybe ?
		for (auto r : s_AvailableRenderers)
		{
			delete r;
		}
	}
	
}


