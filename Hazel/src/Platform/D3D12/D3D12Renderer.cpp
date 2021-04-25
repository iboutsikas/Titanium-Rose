#include "hzpch.h"
#include "D3D12Renderer.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12ForwardRenderer.h"
#include "Platform/D3D12/DecoupledRenderer.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/Profiler/Profiler.h"

#include "Platform/D3D12/CommandQueue.h"
#include "Platform/D3D12/CommandContext.h"

#include "glm/gtc/type_ptr.hpp"

#include <memory>
#include <ImGui/imgui.h>

#include "WinPixEventRuntime/pix3.h"

#define MAX_RESOURCES       100
#define MAX_RENDERTARGETS   50

DECLARE_SHADER(ClearUAV);
DECLARE_SHADER(Dilate);

struct QuadVertex {
	glm::vec3 Position;
};

namespace Hazel {
    D3D12Context*	D3D12Renderer::Context;
    ShaderLibrary*	D3D12Renderer::g_ShaderLibrary;
    TextureLibrary*	D3D12Renderer::g_TextureLibrary;
    D3D12TilePool*	D3D12Renderer::TilePool;

	CommandListManager D3D12Renderer::CommandQueueManager;


    D3D12DescriptorHeap* D3D12Renderer::s_ResourceDescriptorHeap;
	D3D12DescriptorHeap* D3D12Renderer::s_RenderTargetDescriptorHeap;
	D3D12DescriptorHeap* D3D12Renderer::s_DepthStencilDescriptorHeap;
	D3D12VertexBuffer* D3D12Renderer::s_FullscreenQuadVB;
	D3D12IndexBuffer* D3D12Renderer::s_FullscreenQuadIB;

	D3D12Renderer::CommonData D3D12Renderer::s_CommonData;

	Scope<D3D12UploadBuffer<D3D12Renderer::RendererLight>> D3D12Renderer::s_LightsBuffer;
	HeapAllocationDescription D3D12Renderer::s_LightsBufferAllocation;
	HeapAllocationDescription D3D12Renderer::s_ImGuiAllocation;


	D3D12_INPUT_ELEMENT_DESC D3D12Renderer::s_InputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Binormal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Hazel::Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

    uint32_t D3D12Renderer::s_InputLayoutCount = _countof(s_InputLayout);


    uint32_t D3D12Renderer::s_CurrentFrameBuffer = 0;
	uint64_t D3D12Renderer::s_FrameCount = 0;


    std::vector<Ref<FrameBuffer>> D3D12Renderer::s_Framebuffers;
	std::vector<Ref<HGameObject>> D3D12Renderer::s_ForwardOpaqueObjects;
	std::vector<Ref<HGameObject>> D3D12Renderer::s_ForwardTransparentObjects;
	std::vector<Ref<HGameObject>> D3D12Renderer::s_DecoupledOpaqueObjects;

	std::vector<D3D12Renderer*> D3D12Renderer::s_AvailableRenderers;

    void D3D12Renderer::PrepareBackBuffer(CommandContext& context, glm::vec4 clear)
	{
		
		Ref<FrameBuffer> framebuffer = ResolveFrameBuffer();

		ColorBuffer& backbuffer = Context->GetCurrentBackBuffer();

		context.TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		context.TransitionResource(*(framebuffer->ColorResource), D3D12_RESOURCE_STATE_RENDER_TARGET);
		context.FlushResourceBarriers();
		
		context.GetCommandList()->ClearRenderTargetView(framebuffer->RTVAllocation.CPUHandle, &clear.x, 0, nullptr);
		context.GetCommandList()->ClearDepthStencilView(framebuffer->DSVAllocation.CPUHandle,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			1.0f, 0, 0, nullptr
		);
	}
	static uint64_t s_FrameProfileIndex = -1;

    void D3D12Renderer::BeginFrame()
	{
		Context->NewFrame();

        s_ForwardTransparentObjects.clear();
        s_ForwardOpaqueObjects.clear();
        s_DecoupledOpaqueObjects.clear();

	}

    void D3D12Renderer::EndFrame()
    {
#if 0
        r->MainCommandList->Get()->ResourceBarrier(1, &barrier);
		Profiler::EndBlock(r->MainCommandList);


		if (r->DecoupledCommandList->ShouldExecute()) {
			r->DecoupledCommandList->ExecuteAndWait(r->CommandQueue);
		}
		else if (!r->DecoupledCommandList->IsClosed()) {
			r->DecoupledCommandList->Close();
		}

        if (r->WorkerCommandList->ShouldExecute()) {
            r->WorkerCommandList->ExecuteAndWait(r->CommandQueue);
        }
		else if (!r->WorkerCommandList->IsClosed()) {
			r->WorkerCommandList->Close();
		}

        r->MainCommandList->ExecuteAndWait(r->CommandQueue);
#endif
		for (auto renderer : s_AvailableRenderers) {
			renderer->ImplOnFrameEnd();
		}
    }

    void D3D12Renderer::Present()
	{
		GraphicsContext& context = GraphicsContext::Begin("Present");
		ColorBuffer& backbuffer = Context->GetCurrentBackBuffer();
		context.TransitionResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT);
		context.Finish(true);
		s_FrameCount++;
		Context->SwapBuffers();
	}

	void D3D12Renderer::BeginScene(Scene& scene)
	{
		s_CommonData.Scene = &scene;
		s_CommonData.NumLights = 0;
		for (size_t i = 0; i < scene.Lights.size(); i++)
		{
			auto& l = scene.Lights[i];

			RendererLight rl;
			rl.Color = l->Color;
			rl.Position = glm::vec4(l->gameObject->Transform.Position(), 1.0f);
			rl.Range = l->Range;
			rl.Intensity = l->Intensity;
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
		Context->Viewport.TopLeftX = x;
		Context->Viewport.TopLeftY = y;
		Context->Viewport.Width = width;
		Context->Viewport.Height = height;
		Context->Viewport.MinDepth = 0.0f;
		Context->Viewport.MaxDepth = 1.0f;

		Context->ScissorRect.left = x;
		Context->ScissorRect.top = y;
		Context->ScissorRect.right = width;
		Context->ScissorRect.bottom = height;

		Context->WaitForGpu();
		
		GraphicsContext& ctx = GraphicsContext::Begin();

		Context->ResetBackbuffers();
		Context->ResizeSwapChain();
		Context->CreateBackBuffers();

		for (int i = 0; i < Context->DeviceResources->BackBuffers.size(); i++)
		{
			ColorBuffer& buffer = Context->DeviceResources->BackBuffers[i];
			HeapAllocationDescription& alloc = buffer.GetRTV();
			HZ_CORE_ASSERT(alloc.Allocated, "The RTV should be allocated by now");
			CreateRTV(buffer, alloc);
		}


		for (int i = 0; i < s_Framebuffers.size(); i++)
		{
			auto fb = s_Framebuffers[i];
			s_ResourceDescriptorHeap->Release(fb->SRVAllocation);
			s_CommonData.StaticResources--;
			s_RenderTargetDescriptorHeap->Release(fb->RTVAllocation);
			s_CommonData.StaticRenderTargets--;
			s_DepthStencilDescriptorHeap->Release(fb->DSVAllocation);

			fb.reset();
		}

		CreateFrameBuffers(ctx);
		ctx.Finish(true);
	}

	void D3D12Renderer::SetVCsync(bool enable)
	{
		Context->SetVSync(enable);
	}

	void D3D12Renderer::AddStaticResource(Ref<Texture> texture)
	{
		if (s_CommonData.DynamicResources != 0) {
			HZ_CORE_WARN("Tried to allocate static resource after a dynamic resource");
			return;
		}

		if (texture->SRVAllocation.Allocated)
		{
			return;
		}

		texture->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		HZ_CORE_ASSERT(texture->SRVAllocation.Allocated, "Could not allocate space on the resource heap");
		CreateSRV(texture, 0, texture->GetMipLevels(), false);
		//D3D12Renderer::Context->DeviceResources->Device->CreateShaderResourceView(
		//	texture->GetResource(),
		//	nullptr,
		//	texture->SRVAllocation.CPUHandle
		//);
		s_CommonData.StaticResources++;
	}

	void D3D12Renderer::AddDynamicResource(Ref<Texture> texture)
	{

		texture->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		HZ_CORE_ASSERT(texture->SRVAllocation.Allocated, "Could not allocate space on the resource heap");
		
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
			texture->SRVAllocation.CPUHandle
		);
		s_CommonData.DynamicResources++;
	}

	void D3D12Renderer::ReleaseDynamicResource(Ref<Texture> texture)
	{
		s_ResourceDescriptorHeap->Release(texture->SRVAllocation);
	}

	void D3D12Renderer::AddStaticRenderTarget(Ref<Texture> texture)
	{
	}

	void D3D12Renderer::Submit(Ref<HGameObject> gameObject)
	{
        if (gameObject->Mesh != nullptr)
        {
			if (gameObject->Material->IsTransparent) {
				s_ForwardTransparentObjects.push_back(gameObject);
			}
			else if (gameObject->DecoupledComponent.UseDecoupledTexture) {
				s_DecoupledOpaqueObjects.push_back(gameObject);
			}
			else {
				s_ForwardOpaqueObjects.push_back(gameObject);
			}
        }

        for (auto& c : gameObject->children)
        {
            Submit(c);
        }
	}

	void D3D12Renderer::RenderSubmitted(GraphicsContext& gfxContext)
	{
		//s_AvailableRenderers[RendererType::RendererType_Forward]->ImplRenderSubmitted();
		for (auto& renderer : s_AvailableRenderers)
		{
			renderer->ImplRenderSubmitted(gfxContext);
		}
	}

	void D3D12Renderer::ShadeDecoupled()
	{
		DecoupledRenderer* renderer = dynamic_cast<DecoupledRenderer*>(s_AvailableRenderers[RendererType_TextureSpace]);
		CreateMissingVirtualTextures();

		D3D12Renderer::UpdateVirtualTextures();

		GraphicsContext& decoupledContext = GraphicsContext::Begin("Decoupled Shading");
        renderer->ImplRenderVirtualTextures(decoupledContext);
        renderer->ImplDilateVirtualTextures(decoupledContext);
		{
            //ScopedTimer timer("Generate Mips", commandList);

            for (auto obj : s_DecoupledOpaqueObjects)
            {
                auto vtex = obj->DecoupledComponent.VirtualTexture;
                auto tex = std::static_pointer_cast<Texture>(vtex);
                auto mips = vtex->GetMipsUsed();
                GenerateMips(decoupledContext, tex, mips.FinestMip);
            }
		}
		decoupledContext.Finish(true);
	}

	void D3D12Renderer::RenderSkybox(GraphicsContext& gfxContext, uint32_t miplevel)
	{
		
		//ScopedTimer timer("SkyBox", Context->DeviceResources->MainCommandList);

		auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("skybox");
		
		auto framebuffer = ResolveFrameBuffer();

		auto camera = s_CommonData.Scene->Camera;
		struct {
			glm::mat4 ViewInverse;
			glm::mat4 ProjectionTranspose;
			uint32_t MipLevel;
		} PassData;

		PassData.ViewInverse = glm::inverse(camera->GetViewMatrix());
		PassData.ProjectionTranspose = glm::inverse(camera->GetProjectionMatrix());
		PassData.MipLevel = miplevel;

		auto& env = s_CommonData.Scene->Environment;

		auto vb = s_FullscreenQuadVB->GetView();
		vb.StrideInBytes = sizeof(QuadVertex);
		auto ib = s_FullscreenQuadIB->GetView();

		auto r = Context->DeviceResources.get();
		auto fr = Context->CurrentFrameResource;

		gfxContext.TransitionResource(*env.EnvironmentMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		gfxContext.TransitionResource(*(framebuffer->ColorResource), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

		gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &vb);
		gfxContext.GetCommandList()->IASetIndexBuffer(&ib);
		gfxContext.GetCommandList()->OMSetRenderTargets(1, &framebuffer->RTVAllocation.CPUHandle, TRUE, &framebuffer->DSVAllocation.CPUHandle);
		gfxContext.GetCommandList()->RSSetViewports(1, &Context->Viewport);
		gfxContext.GetCommandList()->RSSetScissorRects(1, &Context->ScissorRect);
		gfxContext.GetCommandList()->SetGraphicsRootSignature(shader->GetRootSignature());
		gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
		gfxContext.GetCommandList()->SetGraphicsRoot32BitConstants(0, sizeof(PassData) / sizeof(float), &PassData, 0);
		gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(1, env.EnvironmentMap->SRVAllocation.GPUHandle);

		gfxContext.GetCommandList()->DrawIndexedInstanced(s_FullscreenQuadIB->GetCount(), 1, 0, 0, 0);
	}

	void D3D12Renderer::RenderDiagnostics()
	{
        ImGui::Text("Loaded Textures: %d", D3D12Renderer::g_TextureLibrary->TextureCount());
		auto stats = TilePool->GetStats();
		//ImGui::Text("Heap pages: %d", stats.size());

		uint32_t level = 0;

		if (ImGui::TreeNode(&level, "Heap pages: %d", stats.size()))
		{
			for (auto& s : stats)
			{
				level++;
				if (ImGui::TreeNode(&level, "Page: %d", level))
				{
					ImGui::Text("Max Tiles: %d", s.MaxTiles);
					ImGui::Text("Free Tiles: %d", s.FreeTiles);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

	}

	static Hazel::Scene s_DefaultScene(0.8f);

	void D3D12Renderer::DoToneMapping(GraphicsContext& gfxContext)
	{
		//ScopedTimer timer("Tonemap", Context->DeviceResources->MainCommandList);

		auto framebuffer = ResolveFrameBuffer();
		ColorBuffer& backbuffer = Context->GetCurrentBackBuffer();
		auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("ToneMap");

		if (!s_CommonData.Scene) {
			s_CommonData.Scene = &s_DefaultScene;
		}

		gfxContext.TransitionResource(*(framebuffer->ColorResource), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		gfxContext.TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

		gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfxContext.GetCommandList()->OMSetRenderTargets(1, &(backbuffer.GetRTV().CPUHandle), true, nullptr);
		gfxContext.GetCommandList()->RSSetViewports(1, &Context->Viewport);
		gfxContext.GetCommandList()->RSSetScissorRects(1, &Context->ScissorRect);
		gfxContext.GetCommandList()->SetGraphicsRootSignature(shader->GetRootSignature());
		gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
		gfxContext.GetCommandList()->SetGraphicsRoot32BitConstants(0, 1, &s_CommonData.Scene->Exposure, 0);
		gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(1, framebuffer->SRVAllocation.GPUHandle);
		gfxContext.GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	std::pair<Ref<D3D12TextureCube>, Ref<D3D12TextureCube>> D3D12Renderer::LoadEnvironmentMap(std::string& path)
	{
		std::string envPath = path + "::env";
		std::string irrPath = path + "::ir";

		if (g_TextureLibrary->Exists(envPath))
		{
			auto envTex = g_TextureLibrary->GetAs<D3D12TextureCube>(envPath);
			auto irTex = g_TextureLibrary->GetAs<D3D12TextureCube>(irrPath);

			return { envTex, irTex };
		}


		std::string extension = path.substr(path.find_last_of(".") + 1);

		if (extension != "hdr")
		{
			HZ_CORE_ASSERT(false, "Environment maps must be in HDR format");
			return { nullptr, nullptr };
#if 0
			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			batch.Begin();

			D3D12Texture::TextureCreationOptions opts;
			opts.Path = path;
			opts.Flags = D3D12_RESOURCE_FLAG_NONE;
			auto tex = D3D12TextureCube::Create(batch, opts);

			batch.End(Context->DeviceResources->CommandQueue.Get());

			TextureLibrary->Add(tex);

			return { tex, nullptr };
#endif
		}

		Ref<Texture2D> equirectangularImage = nullptr;
		Ref<D3D12TextureCube> envUnfiltered = nullptr;
		Ref<D3D12TextureCube> envFiltered = nullptr;
		Ref<D3D12TextureCube> envIrradiance = nullptr;
		
		constexpr uint32_t EnvmapSize = 2048;
		constexpr uint32_t IrrmapSize = 32;

        auto r = Context->DeviceResources.get();
        auto fr = Context->CurrentFrameResource;

		// Load the equirect image and create the unfiltered cube texture
		GraphicsContext& gfxContext = GraphicsContext::Begin();
		{
			{
				TextureCreationOptions opts;
				opts.Path = path;
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				opts.MipLevels = 1;
				equirectangularImage = Texture2D::CreateCommittedTexture(opts);
				gfxContext.TransitionResource(*equirectangularImage, D3D12_RESOURCE_STATE_COMMON, true);
				CreateSRV(std::static_pointer_cast<Texture>(equirectangularImage));
			}

			{
				TextureCreationOptions opts;
				opts.Name = "Env Unfiltered";
				opts.Width = EnvmapSize;
				opts.Height = EnvmapSize;
				opts.Depth = 6;
				opts.MipLevels = D3D12::CalculateMips(EnvmapSize, EnvmapSize);
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				envUnfiltered = D3D12TextureCube::Initialize(opts);
				gfxContext.TransitionResource(*envUnfiltered, D3D12_RESOURCE_STATE_COMMON, true);
				CreateUAV(std::static_pointer_cast<Texture>(envUnfiltered), 0);
			}

			{
				TextureCreationOptions opts;
				opts.Name = envPath;
				opts.Width = EnvmapSize;
				opts.Height = EnvmapSize;
				opts.Depth = 6;
				opts.MipLevels = D3D12::CalculateMips(EnvmapSize, EnvmapSize);
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				envFiltered = D3D12TextureCube::Initialize(opts);
				gfxContext.TransitionResource(*envFiltered, D3D12_RESOURCE_STATE_COMMON);
			}

			{
				TextureCreationOptions opts;
				opts.Name = irrPath;
				opts.Width = IrrmapSize;
				opts.Height = IrrmapSize;
				opts.Depth = 6;
				opts.MipLevels = 1;
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				envIrradiance = D3D12TextureCube::Initialize(opts);
				gfxContext.TransitionResource(*envIrradiance, D3D12_RESOURCE_STATE_COMMON);
			}

		}
		
		// Convert equirect to cube and filter
		{	
			auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("Equirectangular2Cube");

			gfxContext.TransitionResource(*envUnfiltered, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

			gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
			gfxContext.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());
			gfxContext.GetCommandList()->SetComputeRootDescriptorTable(0, equirectangularImage->SRVAllocation.GPUHandle); // Source Texture
			gfxContext.GetCommandList()->SetComputeRootDescriptorTable(1, envUnfiltered->UAVAllocation.GPUHandle); // Destination Texture
			uint32_t threadsX = envUnfiltered->GetWidth() / IrrmapSize;
			uint32_t threadsY = envUnfiltered->GetHeight() / IrrmapSize;
			uint32_t threadsZ = 6;
			gfxContext.GetCommandList()->Dispatch(threadsX, threadsY, threadsZ);
			gfxContext.TransitionResource(*envUnfiltered, D3D12_RESOURCE_STATE_COMMON);

			GenerateMips(gfxContext, std::static_pointer_cast<Texture>(envUnfiltered), 0);		
		}
		gfxContext.Finish(true);

		// Prefilter env map
		{
			HeapValidationMark mark(s_ResourceDescriptorHeap);

			auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("EnvironmentPrefilter");
			CreateSRV(std::static_pointer_cast<Texture>(envUnfiltered));
			
			GraphicsContext& context = GraphicsContext::Begin();

			context.TransitionResource(*envFiltered, D3D12_RESOURCE_STATE_COPY_DEST, true);
			context.TransitionResource(*envUnfiltered, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

			// 6 faces
			for (size_t i = 0; i < 6; i++)
			{
				uint32_t index = D3D12CalcSubresource(0, i, 0, envFiltered->GetMipLevels(), 6);
				context.CopySubresource(*envFiltered, index, *envUnfiltered, index);
			}

			context.TransitionResource(*envFiltered, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			context.TransitionResource(*envUnfiltered, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			
			context.GetCommandList()->SetPipelineState(shader->GetPipelineState());
			context.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());
			context.GetCommandList()->SetComputeRootDescriptorTable(2, envUnfiltered->SRVAllocation.GPUHandle);

			const float roughness = 1.0f / std::max<float>((float)envFiltered->GetMipLevels() - 1.0f, 1.0f);
			uint32_t size = (EnvmapSize >> 1);
			
			for (uint32_t level = 1; level < envFiltered->GetMipLevels(); level++ ) 
			{
				uint32_t dispatchGroups = std::max<uint32_t>(1, size / IrrmapSize);
				float levelRoughness = level * roughness;

				HeapAllocationDescription allocation = s_ResourceDescriptorHeap->Allocate(1);
				context.TrackAllocation(allocation);

				CreateUAV(std::static_pointer_cast<Texture>(envFiltered), allocation, level);

				context.GetCommandList()->SetComputeRootDescriptorTable(1, allocation.GPUHandle);
				context.GetCommandList()->SetComputeRoot32BitConstants(0, 1, &levelRoughness, 0);
				context.GetCommandList()->Dispatch(dispatchGroups, dispatchGroups, 6);

				size = (size >> 1);
			}
			context.TransitionResource(*envFiltered, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context.Finish(true);
			s_ResourceDescriptorHeap->Release(envUnfiltered->SRVAllocation);
		}

		// Create IR map
		{
			HeapValidationMark mark(s_ResourceDescriptorHeap);
			auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("EnvironmentIrradiance");
			auto srvAllocation = s_ResourceDescriptorHeap->Allocate(1);
			CreateUAV(std::static_pointer_cast<Texture>(envIrradiance), 0);
			CreateSRV(std::static_pointer_cast<Texture>(envFiltered), srvAllocation, 0, envFiltered->GetMipLevels(), false);

			ComputeContext& computeContext = ComputeContext::Begin("", true);

			computeContext.TransitionResource(*envIrradiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			computeContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
			computeContext.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());
			
			computeContext.GetCommandList()->SetComputeRootDescriptorTable(0, envIrradiance->UAVAllocation.GPUHandle);
			computeContext.GetCommandList()->SetComputeRootDescriptorTable(1, srvAllocation.GPUHandle);

			uint32_t threadsX = envIrradiance->GetWidth() / IrrmapSize;
			uint32_t threadsY = envIrradiance->GetHeight() / IrrmapSize;
			uint32_t threadsZ = envIrradiance->GetDepth();
			computeContext.GetCommandList()->Dispatch(threadsX, threadsY, threadsZ);

			computeContext.TransitionResource(*envIrradiance, D3D12_RESOURCE_STATE_COMMON);

			computeContext.Finish(true);

			s_ResourceDescriptorHeap->Release(envIrradiance->UAVAllocation);
			s_ResourceDescriptorHeap->Release(srvAllocation);
		}

		s_ResourceDescriptorHeap->Release(envUnfiltered->UAVAllocation);
		s_ResourceDescriptorHeap->Release(equirectangularImage->SRVAllocation);

		equirectangularImage.reset();
		envUnfiltered.reset();
		return { envFiltered, envIrradiance};
	}

	Ref<FrameBuffer> D3D12Renderer::ResolveFrameBuffer()
	{
		return s_Framebuffers[Context->m_CurrentBackbufferIndex];
	}

	void D3D12Renderer::Init()
	{
		auto& wnd = Application::Get().GetWindow();
		Context = new D3D12Context();
		Context->Init(&wnd);
        D3D12Renderer::CommandQueueManager.Initialize(Context->DeviceResources->Device.Get());
		Context->CreateSwapChain(CommandQueueManager.GetGraphicsQueue().GetRawPtr());

		D3D12Renderer::g_ShaderLibrary = new Hazel::ShaderLibrary(D3D12Renderer::FrameLatency);
		D3D12Renderer::g_TextureLibrary = new Hazel::TextureLibrary("");
		D3D12Renderer::TilePool = new Hazel::D3D12TilePool();


		auto r = Context->DeviceResources.get();
		auto fr = Context->CurrentFrameResource;

#pragma region Renderer Implementations
		// Add the two renderer implementations
		{
			s_AvailableRenderers.resize(RendererType_Count);
			s_AvailableRenderers[RendererType_Forward] = new D3D12ForwardRenderer();
			s_AvailableRenderers[RendererType_Forward]->ImplOnInit();
			s_AvailableRenderers[RendererType_TextureSpace] = new DecoupledRenderer();
			s_AvailableRenderers[RendererType_TextureSpace]->ImplOnInit();
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

			s_DepthStencilDescriptorHeap = new D3D12DescriptorHeap(
				D3D12Renderer::Context->DeviceResources->Device.Get(),
				D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				10
			);
		}
#pragma endregion

#pragma region Engine Shaders
		// Tone mapping
		{
			D3D12_RT_FORMAT_ARRAY rtvFormats = {};
			rtvFormats.NumRenderTargets = 1;
			rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
			rasterizer.FrontCounterClockwise = true;

			CD3DX12_PIPELINE_STATE_STREAM pipelineStateStream;

			pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			//pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
			pipelineStateStream.RTVFormats = rtvFormats;
			pipelineStateStream.RasterizerState = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

			CD3DX12_DEPTH_STENCIL_DESC1 depthDesc(D3D12_DEFAULT);
			depthDesc.DepthEnable = false;

			pipelineStateStream.DepthStencilState = CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1(depthDesc);

			auto shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/ToneMap.hlsl", pipelineStateStream);
			HZ_CORE_ASSERT(shader, "Shader was null");
			D3D12Renderer::g_ShaderLibrary->Add(shader);
		}
		// Skybox
		{
			static D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			D3D12_RT_FORMAT_ARRAY rtvFormats = {};
			rtvFormats.NumRenderTargets = 1;
			rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

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
			HZ_CORE_ASSERT(shader, "Shader was null");
			D3D12Renderer::g_ShaderLibrary->Add(shader);
		}

		// Equirectangular to Cube
		{
			CD3DX12_PIPELINE_STATE_STREAM stream;

			auto shader = CreateRef<D3D12Shader>("assets/shaders/Equirectangular2Cube.hlsl", stream, ShaderType::Compute);
			HZ_CORE_ASSERT(shader, "Shader was null");
			D3D12Renderer::g_ShaderLibrary->Add(shader);
		}

		// Compute shaders
		{
			CD3DX12_PIPELINE_STATE_STREAM stream;
			const std::string Shaders[] = {
				"assets/shaders/MipGeneration-Linear.hlsl",
				"assets/shaders/MipGeneration-Array.hlsl",
				"assets/shaders/EnvironmentPrefilter.hlsl",
				"assets/shaders/EnvironmentIrradiance.hlsl",
				"assets/shaders/SPBRDF_LUT.hlsl",
				ShaderPathClearUAV,
				ShaderPathDilate
			};

			for (auto& s : Shaders) {
				auto shader = CreateRef<D3D12Shader>(s, stream, ShaderType::Compute);
				HZ_CORE_ASSERT(shader, "Shader was null");
				D3D12Renderer::g_ShaderLibrary->Add(shader);
			}
		}

      
		GraphicsContext& context = GraphicsContext::Begin();
		// Create SPBRDF_LUT
		{
			auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("SPBRDF_LUT");

			TextureCreationOptions opts;
			opts.Name = "spbrdf";
			opts.Width = 256;
			opts.Height = 256;
			opts.Depth = 1;
			opts.Format = DXGI_FORMAT_R16G16_FLOAT;
			opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			opts.MipLevels = 1;

			auto tex = Texture2D::CreateCommittedTexture(opts);

			context.TransitionResource(*tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);

			CreateUAV(std::static_pointer_cast<Texture>(tex), 0);

			context.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());
			context.GetCommandList()->SetPipelineState(shader->GetPipelineState());

			context.GetCommandList()->SetPipelineState(shader->GetPipelineState());
			//ID3D12DescriptorHeap* heaps[] = {
			//   s_ResourceDescriptorHeap->GetHeap()
			//};
			//context.GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);
			context.GetCommandList()->SetComputeRootDescriptorTable(0, tex->UAVAllocation.GPUHandle);
			context.GetCommandList()->Dispatch(tex->GetWidth() / 32, tex->GetHeight() / 32, 1);

			context.TransitionResource(*tex, D3D12_RESOURCE_STATE_COMMON, false);
			g_TextureLibrary->Add(tex);
		}

		std::vector<QuadVertex> verts(4);
		verts[0] = { { -1.0f , -1.0f, 0.1f } };
		verts[1] = { {  1.0f , -1.0f, 0.1f } };
		verts[2] = { {  1.0f ,  1.0f, 0.1f } };
		verts[3] = { { -1.0f ,  1.0f, 0.1f } };

		std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

		s_FullscreenQuadVB = new D3D12VertexBuffer(context, (float*)verts.data(), verts.size() * sizeof(QuadVertex));
		s_FullscreenQuadIB = new D3D12IndexBuffer(context, indices.data(), indices.size());

		context.Finish(true);

		auto t = g_TextureLibrary->Get(std::string("spbrdf"));
		s_ResourceDescriptorHeap->Release(t->UAVAllocation);


#pragma endregion

#pragma FrameBuffers
		{
			Context->CreateBackBuffers();
            for (int i = 0; i < Context->DeviceResources->BackBuffers.size(); i++)
            {
                ColorBuffer& buffer = Context->DeviceResources->BackBuffers[i];
				HeapAllocationDescription allocation = s_RenderTargetDescriptorHeap->Allocate(1);
				buffer.SetRTV(allocation);         
				CreateRTV(buffer, allocation);
            }

			s_Framebuffers.resize(D3D12Renderer::FrameLatency);
			GraphicsContext& context = GraphicsContext::Begin();
			CreateFrameBuffers(context);
			context.Finish(true);
		}
#pragma endregion

#pragma region Scene Lights Buffer
		// Create the buffer for the scene lights
		{
			HZ_CORE_ASSERT((sizeof(RendererLight) % 16) == 0, "The size of the light struct should be 128-bit aligned");

			s_LightsBuffer = CreateScope<D3D12UploadBuffer<RendererLight>>(MaxSupportedLights, false);
			s_LightsBuffer->ResourceRaw()->SetName(L"Lights buffer");

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
				s_LightsBuffer->ResourceRaw(),
				&desc,
				s_LightsBufferAllocation.CPUHandle
			);
		}
#pragma endregion
		
		s_ImGuiAllocation = s_ResourceDescriptorHeap->Allocate(1);
		s_CommonData.StaticResources++;

	}

	void D3D12Renderer::Shutdown()
	{
		Context->WaitForGpu();
        s_ForwardTransparentObjects.clear();
        s_ForwardOpaqueObjects.clear();
        s_DecoupledOpaqueObjects.clear();
		s_LightsBuffer.release();
		delete s_FullscreenQuadVB;
		delete s_FullscreenQuadIB;

		delete Context;
		delete g_ShaderLibrary;
		delete g_TextureLibrary;
		delete TilePool;

		delete s_ResourceDescriptorHeap;
		delete s_RenderTargetDescriptorHeap;
		delete s_DepthStencilDescriptorHeap;

		// TODO: Flush pipeline maybe ?
		for (auto r : s_AvailableRenderers)
		{
			delete r;
		}
	}

	void D3D12Renderer::ReclaimDynamicDescriptors()
	{
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

	void D3D12Renderer::CreateFrameBuffers(CommandContext& context)
	{
		for (int i = 0; i < s_Framebuffers.size(); i++)
		{
			FrameBufferOptions opts;
			opts.Width = Context->Viewport.Width;
			opts.Height = Context->Viewport.Height;
			opts.ColourFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			opts.DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			opts.Name = "Framebuffer[" + std::to_string(i) + "]";
			s_Framebuffers[i] = FrameBuffer::Create(opts);
			auto fb = s_Framebuffers[i];

			fb->RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
			CreateRTV(std::static_pointer_cast<Texture>(fb->ColorResource), fb->RTVAllocation, 0);
			s_CommonData.StaticRenderTargets++;

			fb->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
			CreateSRV(std::static_pointer_cast<Texture>(fb->ColorResource),
				fb->SRVAllocation, 0, 1);
			s_CommonData.StaticResources++;

			fb->DSVAllocation = s_DepthStencilDescriptorHeap->Allocate(1);
			CreateDSV(std::static_pointer_cast<Texture>(fb->DepthStensilResource), fb->DSVAllocation);

			context.TransitionResource(*fb->ColorResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
			context.TransitionResource(*fb->DepthStensilResource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		}
	}

    void D3D12Renderer::ImplRenderSubmitted(GraphicsContext& gfxContext)
    {

    }

    void D3D12Renderer::ImplOnInit()
    {

    }

    void D3D12Renderer::ImplSubmit(Ref<HGameObject> gameObject)
    {

    }

    void D3D12Renderer::ImplSubmit(D3D12ResourceBatch& batch, Ref<HGameObject> gameObject)
    {

    }

    void D3D12Renderer::CreateUAV(Ref<Texture> texture, uint32_t mip)
	{
		if (!texture->UAVAllocation.Allocated)
		{
			texture->UAVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		}
		
		CreateUAV(texture, texture->UAVAllocation, mip);
	}

	void D3D12Renderer::CreateUAV(Ref<Texture> texture, HeapAllocationDescription& description, uint32_t mip)
	{
		auto desc = texture->GetResource()->GetDesc();

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = desc.Format;

		if (desc.DepthOrArraySize > 1)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = mip;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
		}
		else
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = mip;
		}

		Context->DeviceResources->Device->CreateUnorderedAccessView(
			texture->GetResource(),
			nullptr,
			&uavDesc,
			description.CPUHandle
		);
	}

	void D3D12Renderer::CreateSRV(Ref<Texture> texture, uint32_t mostDetailedMip, uint32_t mips, bool forceArray)
	{
		if (!texture->SRVAllocation.Allocated)
		{
			texture->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		}
		
		CreateSRV(texture, texture->SRVAllocation, mostDetailedMip, mips, forceArray);
	}

	void D3D12Renderer::CreateSRV(Ref<Texture> texture, HeapAllocationDescription& description, uint32_t mostDetailedMip, uint32_t mips, bool forceArray)
	{
		CreateSRV(*texture, description, mostDetailedMip, mips, forceArray);
	}

	void D3D12Renderer::CreateSRV(Texture& resource, uint32_t mostDetailedMip, uint32_t mips, bool forceArray)
	{
		if (!resource.SRVAllocation.Allocated) {
			resource.SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		}
		CreateSRV(resource, resource.SRVAllocation, mostDetailedMip, mips, forceArray);
	}

	void D3D12Renderer::CreateSRV(Texture& resource, HeapAllocationDescription& description, uint32_t mostDetailedMip, uint32_t mips, bool forceArray)
	{
        auto desc = resource.GetResource()->GetDesc();

        uint32_t actuallMipLevels = (mips > 0) ? mips : desc.MipLevels - mostDetailedMip;

        D3D12_SRV_DIMENSION srvDim;
        if (desc.DepthOrArraySize == 1) {
            srvDim = D3D12_SRV_DIMENSION_TEXTURE2D;
        }
        else if (desc.DepthOrArraySize == 6 && !forceArray) {
            srvDim = D3D12_SRV_DIMENSION_TEXTURECUBE;
        }
        else {
            srvDim = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = srvDim;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (srvDim) {
        case D3D12_SRV_DIMENSION_TEXTURE2D:
            srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture2D.MipLevels = actuallMipLevels;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture2DArray.MipLevels = actuallMipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
            assert(desc.DepthOrArraySize == 6);
            srvDesc.TextureCube.MostDetailedMip = mostDetailedMip;
            srvDesc.TextureCube.MipLevels = actuallMipLevels;
            break;
        }
        Context->DeviceResources->Device->CreateShaderResourceView(
            resource.GetResource(),
            &srvDesc,
            description.CPUHandle
        );
	}

	void D3D12Renderer::CreateRTV(Ref<Texture> texture, uint32_t mip)
	{
		if (!texture->RTVAllocation.Allocated)
		{
			texture->RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
		}

		CreateRTV(texture, texture->RTVAllocation, mip);
	}

	void D3D12Renderer::CreateRTV(Ref<Texture> texture, HeapAllocationDescription& description, uint32_t mip /* = 0 */) {
		CreateRTV(*texture, description, mip);
	}

	void D3D12Renderer::CreateRTV(Texture& texture, uint32_t mip)
	{
		if (!texture.RTVAllocation.Allocated) {
			texture.RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
		}
		CreateRTV(texture, texture.RTVAllocation, mip);
	}

	void D3D12Renderer::CreateRTV(Texture& texture, HeapAllocationDescription& description, uint32_t mip)
	{
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = texture.GetFormat();
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = mip;

        GetDevice()->CreateRenderTargetView(
			texture.GetResource(),
            &rtvDesc,
            description.CPUHandle
        );
	}


	void D3D12Renderer::CreateRTV(GpuResource& resource, HeapAllocationDescription& description, uint32_t mip)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = resource.GetFormat();
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = mip;

		GetDevice()->CreateRenderTargetView(
			resource.GetResource(),
			&rtvDesc,
			description.CPUHandle
		);
	}

	void D3D12Renderer::CreateDSV(Ref<Texture> texture, HeapAllocationDescription& description)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = texture->GetFormat();
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		Context->DeviceResources->Device->CreateDepthStencilView(texture->GetResource(), &dsvDesc, description.CPUHandle);
	}

	void D3D12Renderer::CreateMissingVirtualTextures()
	{
		for (auto& obj : s_DecoupledOpaqueObjects)
		{
			if (obj->DecoupledComponent.VirtualTexture != nullptr)
				continue;

            uint32_t width, height, mips;
            std::string name;
            auto albedo = obj->Material->AlbedoTexture;

            if (albedo != nullptr)
            {
                width = albedo->GetWidth();
                height = albedo->GetHeight();
                mips = albedo->GetMipLevels();
                name = albedo->GetIdentifier() + "-virtual";
            }
            else
            {
                width = height = 2048;
                mips = D3D12::CalculateMips(width, height);
                name = obj->Material->Name + "-virtual";
            }
            TextureCreationOptions opts;
            opts.Name = name;
            opts.Width = width;
            opts.Height = height;
            opts.MipLevels = mips;
            opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            auto tex = Hazel::Texture2D::CreateVirtualTexture(opts);
            auto fb = Hazel::Texture2D::CreateFeedbackMap(*tex);
            tex->SetFeedbackMap(fb);
            obj->DecoupledComponent.VirtualTexture = std::dynamic_pointer_cast<VirtualTexture2D>(tex);

            tex->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
            tex->RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
            fb->UAVAllocation = s_ResourceDescriptorHeap->Allocate(1);
            auto fbDesc = fb->GetResource()->GetDesc();

            D3D12_UNORDERED_ACCESS_VIEW_DESC fbUAVDesc = {};
            fbUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            fbUAVDesc.Format = fbDesc.Format;
            auto dims = fb->GetDimensions();
            fbUAVDesc.Buffer.NumElements = dims.x * dims.y;
            fbUAVDesc.Buffer.StructureByteStride = sizeof(uint32_t);
            D3D12Renderer::GetDevice()->CreateUnorderedAccessView(
                fb->GetResource(),
                nullptr,
                &fbUAVDesc,
                fb->UAVAllocation.CPUHandle
            );
            CreateSRV(std::static_pointer_cast<Texture>(tex), 0);
            CreateRTV(std::static_pointer_cast<Texture>(tex), 0);
		}
	}

	void D3D12Renderer::GenerateMips(Ref<Texture> texture, uint32_t mostDetailedMip)
	{
        HeapValidationMark mark(s_ResourceDescriptorHeap);
        
		ComputeContext& computeContext = ComputeContext::Begin("", true);
		GenerateMips(computeContext, texture, mostDetailedMip);
		computeContext.Finish(true);
	}

	void D3D12Renderer::GenerateMips(CommandContext& context, Ref<Texture> texture, uint32_t mostDetailedMip)
	{
		//Profiler::BeginBlock(texture->GetIdentifier(), commandList);

		auto srcDesc = texture->GetResource()->GetDesc();
		uint32_t remainingMips = (srcDesc.MipLevels > mostDetailedMip) ? srcDesc.MipLevels - mostDetailedMip - 1 : srcDesc.MipLevels - 1;

		HeapAllocationDescription srcAllocation = s_ResourceDescriptorHeap->Allocate(1);

		HZ_CORE_ASSERT(srcAllocation.Allocated, "Run out of heap space. Something is not releasing properly");
		context.TrackAllocation(srcAllocation);
		// We need this allocation so the view is an Array if we are dealing with a cube
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = srcDesc.Format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (srcDesc.DepthOrArraySize > 1)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = srcDesc.MipLevels;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = srcDesc.DepthOrArraySize;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = srcDesc.MipLevels;
			}

			D3D12Renderer::GetDevice()->CreateShaderResourceView(
				texture->GetResource(),
				&srvDesc,
				srcAllocation.CPUHandle
			);
		}	

		Ref<D3D12Shader> shader = nullptr;
		if (srcDesc.DepthOrArraySize > 1)
		{
			shader = g_ShaderLibrary->GetAs<D3D12Shader>("MipGeneration-Array");
		}
		else
		{
			shader = g_ShaderLibrary->GetAs<D3D12Shader>("MipGeneration-Linear");
		}
		context.TransitionResource(*texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		context.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());
		context.GetCommandList()->SetPipelineState(shader->GetPipelineState());

		std::vector<HeapAllocationDescription> heapAllocations;
		uint32_t allocationCounter = 0;
		for (uint32_t srcMip = mostDetailedMip; srcMip < srcDesc.MipLevels - 1;)
		{
			uint32_t srcWidth	= srcDesc.Width >> srcMip;
			uint32_t srcHeight	= srcDesc.Height >> srcMip;
			uint32_t dstWidth	= static_cast<uint32_t>(srcWidth >> 1);
			uint32_t dstHeight	= static_cast<uint32_t>(srcHeight >> 1);
			uint32_t mipCount = std::min<uint32_t>(MipsPerIteration, remainingMips);

			// Clamp our variables
			mipCount = (srcMip + mipCount) >= srcDesc.MipLevels ? srcDesc.MipLevels - srcMip - 1 : mipCount;
			dstWidth = std::max<uint32_t>(1, dstWidth);
			dstHeight = std::max<uint32_t>(1, dstHeight);

			struct alignas(16) {
				glm::vec4	TexelSize;
				uint32_t	SourceLevel;
				uint32_t	Levels;
				uint64_t	__padding;
			} passData;

            passData.TexelSize = glm::vec4(srcWidth, srcHeight, 1.0f / (float)dstWidth, 1.0f / (float)dstHeight);
			passData.SourceLevel = srcMip;
			passData.Levels = mipCount;


			for (uint32_t mip = 0; mip < mipCount; mip++)
			{
				HeapAllocationDescription allocation = s_ResourceDescriptorHeap->Allocate(1);
				heapAllocations.push_back(allocation);
				CreateUAV(texture, allocation, srcMip + mip + 1);
			}

			if (mipCount < MipsPerIteration)
			{
				for (uint32_t mip = mipCount; mip < MipsPerIteration; mip++)
				{
					HeapAllocationDescription allocation = s_ResourceDescriptorHeap->Allocate(1);
					heapAllocations.push_back(allocation);
					D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
					uavDesc.Format = srcDesc.Format;
					if (srcDesc.DepthOrArraySize > 1)
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						uavDesc.Texture2DArray.MipSlice = 0;
						uavDesc.Texture2DArray.FirstArraySlice = 0;
						uavDesc.Texture2DArray.ArraySize = srcDesc.DepthOrArraySize;
					}
					else
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						uavDesc.Texture2D.MipSlice = 0;
					}

					Context->DeviceResources->Device->CreateUnorderedAccessView(
						nullptr,
						nullptr,
						&uavDesc,
						allocation.CPUHandle
					);
				}
			}

            auto x_count = D3D12::RoundToMultiple(dstWidth, 8);
            auto y_count = D3D12::RoundToMultiple(dstHeight, 8);

			context.GetCommandList()->SetComputeRoot32BitConstants(0, sizeof(passData) / sizeof(uint32_t), &passData, 0);
			context.GetCommandList()->SetComputeRootDescriptorTable(1, srcAllocation.GPUHandle);
			context.GetCommandList()->SetComputeRootDescriptorTable(2, heapAllocations[allocationCounter + 0].GPUHandle);
			context.GetCommandList()->SetComputeRootDescriptorTable(3, heapAllocations[allocationCounter + 1].GPUHandle);
			context.GetCommandList()->SetComputeRootDescriptorTable(4, heapAllocations[allocationCounter + 2].GPUHandle);
			context.GetCommandList()->SetComputeRootDescriptorTable(5, heapAllocations[allocationCounter + 3].GPUHandle);
			context.GetCommandList()->Dispatch(x_count, y_count, srcDesc.DepthOrArraySize);
			context.GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(texture->GetResource()));
			srcMip += mipCount;
			allocationCounter += 4;
		}
		context.TrackAllocation(heapAllocations);
	}

	void D3D12Renderer::ClearUAV(ID3D12GraphicsCommandList* cmdlist, Ref<D3D12FeedbackMap>& resource, uint32_t value)
	{
		auto shader = g_ShaderLibrary->GetAs<D3D12Shader>("ClearUAV");


		cmdlist->SetComputeRootSignature(shader->GetRootSignature());
		cmdlist->SetPipelineState(shader->GetPipelineState());
		ID3D12DescriptorHeap* heaps[] = {
			s_ResourceDescriptorHeap->GetHeap()
		};
		cmdlist->SetDescriptorHeaps(_countof(heaps), heaps);
		cmdlist->SetComputeRoot32BitConstants(0, 1, &value, 0);

		ID3D12Resource* res = nullptr;
		glm::ivec3 dims = { 1, 1, 1 };

		if (resource != nullptr)
		{
			res = resource->GetResource();
			dims = resource->GetDimensions();
		}

		auto dispatch_count = D3D12::RoundToMultiple(dims.x * dims.y, 6);

		cmdlist->Dispatch(dispatch_count, 1, 1);
		cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(res));

	}

	void D3D12Renderer::UpdateVirtualTextures()
	{
		if (s_DecoupledOpaqueObjects.size() == 0)
			return;

		ComputeContext& computeContext = ComputeContext::Begin("Feedback Readback");
		for (auto obj : s_DecoupledOpaqueObjects)
		{
			auto tex = obj->DecoupledComponent.VirtualTexture;
			auto feedback = tex->GetFeedbackMap();
			//ScopedTimer t("Feedback Update", commandList);
			feedback->Update(computeContext);
		}
        computeContext.Finish(true);
		ScopedTimer timer("Tilemaps Update");
        for (auto obj : s_DecoupledOpaqueObjects)
        {
            auto tex = obj->DecoupledComponent.VirtualTexture;
			auto mips = tex->ExtractMipsUsed();
            //ScopedTimer t("Texture Map", commandList);
			TilePool->MapTexture(*tex);
            auto mip = mips.FinestMip >= tex->GetMipLevels() ? tex->GetMipLevels() - 1 : mips.FinestMip;

            CreateSRV(std::static_pointer_cast<Texture>(tex), mip);
		}		
	}

	void D3D12Renderer::RenderVirtualTextures()
	{
		if (s_DecoupledOpaqueObjects.size() == 0)
			return;
		
        auto renderer = dynamic_cast<DecoupledRenderer*>(s_AvailableRenderers[RendererType_TextureSpace]);
		/*renderer->ImplRenderVirtualTextures();
		renderer->ImplDilateVirtualTextures();*/
	}

	void D3D12Renderer::GenerateVirtualMips()
	{
		HeapValidationMark mark(s_ResourceDescriptorHeap);
		//ScopedTimer timer("Generate Mips");

		for (auto obj : s_DecoupledOpaqueObjects)
		{
			auto vtex = obj->DecoupledComponent.VirtualTexture;
			auto tex = std::static_pointer_cast<Texture>(vtex);
			auto mips = vtex->GetMipsUsed();
			GenerateMips(tex, mips.FinestMip);
		}
	}

	void D3D12Renderer::ClearVirtualMaps()
	{
		auto shader = g_ShaderLibrary->GetAs<D3D12Shader>(ShaderNameClearUAV);
		
		ComputeContext& context = ComputeContext::Begin("Clear Virtual Feedback Maps", true);

		//PIXBeginEvent(cmdlist.Get(), PIX_COLOR(1, 0, 1), "Clear feedback");

		context.GetCommandList()->SetPipelineState(shader->GetPipelineState());
		context.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());

		for (auto obj : s_DecoupledOpaqueObjects)
		{
			auto tex = obj->DecoupledComponent.VirtualTexture;
			auto mips = tex->GetMipLevels();
			auto fb = tex->GetFeedbackMap();
			auto dims = fb->GetDimensions();
#if HZ_DEBUG
			if (mips == 1)
			{
				__debugbreak();
			}
#endif
			context.GetCommandList()->SetComputeRoot32BitConstants(0, 1, &mips, 0);
			context.GetCommandList()->SetComputeRootDescriptorTable(1, fb->UAVAllocation.GPUHandle);

			auto dispatch_count = Hazel::D3D12::RoundToMultiple(dims.x * dims.y, 64);

			context.GetCommandList()->Dispatch(dispatch_count, 1, 1);
		}

		context.Finish(true);
	}
	

}


