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
    ShaderLibrary*	D3D12Renderer::ShaderLibrary;
    TextureLibrary*	D3D12Renderer::TextureLibrary;
    D3D12TilePool*	D3D12Renderer::TilePool;

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


    std::vector<Ref<FrameBuffer>> D3D12Renderer::s_Framebuffers;
	std::vector<Ref<GameObject>> D3D12Renderer::s_OpaqueObjects;
	std::vector<Ref<GameObject>> D3D12Renderer::s_TransparentObjects;
	std::vector<Ref<GameObject>> D3D12Renderer::s_DecoupledObjects;

	std::vector<D3D12Renderer*> D3D12Renderer::s_AvailableRenderers;
	D3D12Renderer* D3D12Renderer::s_CurrentRenderer = nullptr;


    void D3D12Renderer::PrepareBackBuffer(glm::vec4 clear)
	{
		D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
		auto cmdList = batch.Begin();
		auto framebuffer = ResolveFrameBuffer();

		auto backbuffer = Context->GetCurrentBackBuffer();

		CD3DX12_RESOURCE_BARRIER transitions[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(backbuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		};
		cmdList->ResourceBarrier(_countof(transitions), transitions);

		framebuffer->ColorResource->Transition(cmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
		
		cmdList->ClearRenderTargetView(framebuffer->RTVAllocation.CPUHandle, &clear.x, 0, nullptr);
		cmdList->ClearDepthStencilView(framebuffer->DSVAllocation.CPUHandle,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			1.0f, 0, 0, nullptr
		);

		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
	}

    void D3D12Renderer::BeginFrame()
	{
		Context->NewFrame();

		ShaderLibrary->Update();
		
		//ReclaimDynamicDescriptors();
		
		Context->DeviceResources->CommandList->OMSetRenderTargets(1, &Context->CurrentBackBufferView(), true, nullptr);

		ID3D12DescriptorHeap* const heaps[] = {
		   s_ResourceDescriptorHeap->GetHeap()
		};
		Context->DeviceResources->CommandList->SetDescriptorHeaps(
			_countof(heaps), heaps
		);

        s_TransparentObjects.clear();
        s_OpaqueObjects.clear();
        s_DecoupledObjects.clear();

	}

    void D3D12Renderer::EndFrame()
    {
		auto backBuffer = Context->GetCurrentBackBuffer();

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        Context->DeviceResources->CommandList->ResourceBarrier(1, &barrier);

        D3D12::ThrowIfFailed(Context->DeviceResources->CommandList->Close());

        ID3D12CommandList* const commandLists[] = {
			Context->DeviceResources->CommandList.Get()
        };
		Context->DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
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
			auto l = scene.Lights[i];

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
		auto r = Context->DeviceResources.get();

		D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
		auto cmdList = batch.Begin();

		Context->CleanupRenderTargetViews();
		Context->ResizeSwapChain();
		Context->CreateRenderTargetViews();
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

		CreateFrameBuffers(batch);

		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
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

	void D3D12Renderer::AddDynamicResource(Ref<D3D12Texture> texture)
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

	void D3D12Renderer::ReleaseDynamicResource(Ref<D3D12Texture> texture)
	{
		s_ResourceDescriptorHeap->Release(texture->SRVAllocation);
	}

	void D3D12Renderer::AddStaticRenderTarget(Ref<D3D12Texture> texture)
	{
	}

	void D3D12Renderer::Submit(Hazel::Ref<Hazel::GameObject> go)
	{
		if (go->Mesh != nullptr)
		{
			for (auto& renderer : s_AvailableRenderers)
			{
				renderer->ImplSubmit(go);
			}
		}

		for (auto& c : go->children)
		{
			Submit(c);
		}
	}

	void D3D12Renderer::Submit(D3D12ResourceBatch& batch, Ref<GameObject> gameObject)
	{
        if (gameObject->Mesh != nullptr)
        {
            for (auto& renderer : s_AvailableRenderers)
            {
                renderer->ImplSubmit(batch, gameObject);
            }
        }

        for (auto& c : gameObject->children)
        {
            Submit(batch, c);
        }
	}

	void D3D12Renderer::RenderSubmitted()
	{
		//s_AvailableRenderers[RendererType::RendererType_Forward]->ImplRenderSubmitted();
		for (auto& renderer : s_AvailableRenderers)
		{
			renderer->ImplRenderSubmitted();
		}
	}

	void D3D12Renderer::RenderSkybox(uint32_t miplevel)
	{
		auto shader = ShaderLibrary->GetAs<D3D12Shader>("skybox");
		
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


		D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
		auto cmdList = batch.Begin();
		PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Skybox pass");

		env.EnvironmentMap->Transition(batch, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &vb);
		cmdList->IASetIndexBuffer(&ib);
		cmdList->OMSetRenderTargets(1, &framebuffer->RTVAllocation.CPUHandle, TRUE, &framebuffer->DSVAllocation.CPUHandle);
		cmdList->RSSetViewports(1, &Context->Viewport);
		cmdList->RSSetScissorRects(1, &Context->ScissorRect);
		ID3D12DescriptorHeap* descriptorHeaps[] = {
			s_ResourceDescriptorHeap->GetHeap()
		};
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
		cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
		cmdList->SetPipelineState(shader->GetPipelineState());
		cmdList->SetGraphicsRoot32BitConstants(0, sizeof(PassData) / sizeof(float), &PassData, 0);
		cmdList->SetGraphicsRootDescriptorTable(1, env.EnvironmentMap->SRVAllocation.GPUHandle);

		cmdList->DrawIndexedInstanced(s_FullscreenQuadIB->GetCount(), 1, 0, 0, 0);

		PIXEndEvent(cmdList.Get());
		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
	}

	void D3D12Renderer::RenderDiagnostics()
	{
        ImGui::Text("Loaded Textures: %d", D3D12Renderer::TextureLibrary->TextureCount());
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

	void D3D12Renderer::DoToneMapping()
	{

		auto framebuffer = ResolveFrameBuffer();
		auto shader = ShaderLibrary->GetAs<D3D12Shader>("ToneMap");

		D3D12ResourceBatch batch(Context->DeviceResources->Device);
		auto cmdList = batch.Begin();
		PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Tonemap pass");
		framebuffer->ColorResource->Transition(cmdList.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->OMSetRenderTargets(1, &Context->CurrentBackBufferView(), true, nullptr);
		cmdList->RSSetViewports(1, &Context->Viewport);
		cmdList->RSSetScissorRects(1, &Context->ScissorRect);
		ID3D12DescriptorHeap* const heaps[] = {
		   s_ResourceDescriptorHeap->GetHeap()
		};
		cmdList->SetDescriptorHeaps(
			_countof(heaps), heaps
		);
		
		cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
		cmdList->SetPipelineState(shader->GetPipelineState());
		cmdList->SetGraphicsRoot32BitConstants(0, 1, &s_CommonData.Scene->Exposure, 0);
		cmdList->SetGraphicsRootDescriptorTable(1, framebuffer->SRVAllocation.GPUHandle);
		cmdList->DrawInstanced(3, 1, 0, 0);
		PIXEndEvent(cmdList.Get());
		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
	}

	std::pair<Ref<D3D12TextureCube>, Ref<D3D12TextureCube>> D3D12Renderer::LoadEnvironmentMap(std::string& path)
	{
		std::string envPath = path + "-env";
		std::string irrPath = path + "-ir";

		if (TextureLibrary->Exists(envPath))
		{
			auto envTex = TextureLibrary->GetAs<D3D12TextureCube>(envPath);
			auto irTex = TextureLibrary->GetAs<D3D12TextureCube>(irrPath);

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



		Ref<D3D12Texture2D> equirectangularImage = nullptr;
		Ref<D3D12TextureCube> envUnfiltered = nullptr;
		Ref<D3D12TextureCube> envFiltered = nullptr;
		Ref<D3D12TextureCube> envIrradiance = nullptr;
		
		constexpr uint32_t EnvmapSize = 2048;
		constexpr uint32_t IrrmapSize = 32;
		// Load the equirect image and create the unfiltered cube texture
		{
			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			auto cmdList = batch.Begin();
			{
				D3D12Texture::TextureCreationOptions opts;
				opts.Path = path;
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				opts.MipLevels = 1;
				equirectangularImage = D3D12Texture2D::CreateCommittedTexture(batch, opts);
				equirectangularImage->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
				CreateSRV(std::static_pointer_cast<D3D12Texture>(equirectangularImage));
			}

			{
				D3D12Texture::TextureCreationOptions opts;
				opts.Name = "Env Unfiltered";
				opts.Width = EnvmapSize;
				opts.Height = EnvmapSize;
				opts.Depth = 6;
				opts.MipLevels = D3D12::CalculateMips(EnvmapSize, EnvmapSize);
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				envUnfiltered = D3D12TextureCube::Create(batch, opts);
				envUnfiltered->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
				CreateUAV(std::static_pointer_cast<D3D12Texture>(envUnfiltered), 0);

			}

			{
				D3D12Texture::TextureCreationOptions opts;
				opts.Name = envPath;
				opts.Width = EnvmapSize;
				opts.Height = EnvmapSize;
				opts.Depth = 6;
				opts.MipLevels = D3D12::CalculateMips(EnvmapSize, EnvmapSize);
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				envFiltered = D3D12TextureCube::Create(batch, opts);
				envFiltered->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
			}

			{
				D3D12Texture::TextureCreationOptions opts;
				opts.Name = irrPath;
				opts.Width = IrrmapSize;
				opts.Height = IrrmapSize;
				opts.Depth = 6;
				opts.MipLevels = 1;
				opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				envIrradiance = D3D12TextureCube::Create(batch, opts);
				envIrradiance->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
			}

			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
		}
		
		// Convert equirect to cube and filter
		{
			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			auto cmdList = batch.Begin();
			PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Equirect 2 Cube: %s", path.c_str());
			auto shader = ShaderLibrary->GetAs<D3D12Shader>("Equirectangular2Cube");

			envUnfiltered->Transition(batch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			ID3D12DescriptorHeap* heaps[] = {
				s_ResourceDescriptorHeap->GetHeap()
			};
			cmdList->SetDescriptorHeaps(1, heaps);
			cmdList->SetPipelineState(shader->GetPipelineState());
			cmdList->SetComputeRootSignature(shader->GetRootSignature());
			cmdList->SetComputeRootDescriptorTable(0, equirectangularImage->SRVAllocation.GPUHandle); // Source Texture
			cmdList->SetComputeRootDescriptorTable(1, envUnfiltered->UAVAllocation.GPUHandle); // Destination Texture
			uint32_t threadsX = envUnfiltered->GetWidth() / IrrmapSize;
			uint32_t threadsY = envUnfiltered->GetHeight() / IrrmapSize;
			uint32_t threadsZ = 6;
			cmdList->Dispatch(threadsX, threadsY, threadsZ);

			envUnfiltered->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
			PIXEndEvent(cmdList.Get());
			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();

			GenerateMips(std::static_pointer_cast<D3D12Texture>(envUnfiltered), 0);		
		}

		// Prefilter env map
		{
			HeapValidationMark mark(s_ResourceDescriptorHeap);

			auto shader = ShaderLibrary->GetAs<D3D12Shader>("EnvironmentPrefilter");
			CreateSRV(std::static_pointer_cast<D3D12Texture>(envUnfiltered));
			
			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			auto cmdList = batch.Begin();
			PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Environment Prefilter: %s", path.c_str());
			envFiltered->Transition(batch, D3D12_RESOURCE_STATE_COPY_DEST);
			envUnfiltered->Transition(batch, D3D12_RESOURCE_STATE_COPY_SOURCE);

			// 6 faces
			for (size_t i = 0; i < 6; i++)
			{
				uint32_t index = D3D12CalcSubresource(0, i, 0, envFiltered->GetMipLevels(), 6);
				cmdList->CopyTextureRegion(
					&CD3DX12_TEXTURE_COPY_LOCATION(envFiltered->GetResource(), index), // Destination
					0, 0, 0,
					&CD3DX12_TEXTURE_COPY_LOCATION(envUnfiltered->GetResource(), index), // Source
					nullptr
				);
			}

			envFiltered->Transition(batch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			envUnfiltered->Transition(batch, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			ID3D12DescriptorHeap* heaps[] = {
				s_ResourceDescriptorHeap->GetHeap()
			};
			cmdList->SetDescriptorHeaps(1, heaps);
			cmdList->SetPipelineState(shader->GetPipelineState());
			cmdList->SetComputeRootSignature(shader->GetRootSignature());
			cmdList->SetComputeRootDescriptorTable(2, envUnfiltered->SRVAllocation.GPUHandle);

			const float roughness = 1.0f / std::max<float>((float)envFiltered->GetMipLevels() - 1.0f, 1.0f);
			uint32_t size = (EnvmapSize >> 1);
			
			std::vector<HeapAllocationDescription> heapAllocations(envFiltered->GetMipLevels());

			for (uint32_t level = 1; level < envFiltered->GetMipLevels(); level++ ) 
			{
				uint32_t dispatchGroups = std::max<uint32_t>(1, size / IrrmapSize);
				float levelRoughness = level * roughness;

				heapAllocations[level] = s_ResourceDescriptorHeap->Allocate(1);

				CreateUAV(std::static_pointer_cast<D3D12Texture>(envFiltered), heapAllocations[level], level);

				cmdList->SetComputeRootDescriptorTable(1, heapAllocations[level].GPUHandle);
				cmdList->SetComputeRoot32BitConstants(0, 1, &levelRoughness, 0);
				cmdList->Dispatch(dispatchGroups, dispatchGroups, 6);

				size = (size >> 1);
			}
			envFiltered->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
			PIXEndEvent(cmdList.Get());
			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();

			for (auto& a : heapAllocations)
			{
				s_ResourceDescriptorHeap->Release(a);
			}
			s_ResourceDescriptorHeap->Release(envUnfiltered->SRVAllocation);
		}

		// Create IR map
		{
			HeapValidationMark mark(s_ResourceDescriptorHeap);
			auto shader = ShaderLibrary->GetAs<D3D12Shader>("EnvironmentIrradiance");
			auto srvAllocation = s_ResourceDescriptorHeap->Allocate(1);
			CreateUAV(std::static_pointer_cast<D3D12Texture>(envIrradiance), 0);
			CreateSRV(std::static_pointer_cast<D3D12Texture>(envFiltered), srvAllocation, 0, envFiltered->GetMipLevels(), false);

			D3D12ResourceBatch batch(Context->DeviceResources->Device);
			auto cmdList = batch.Begin();
			PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Environment Irradiance: %s", path.c_str());
			envIrradiance->Transition(batch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			ID3D12DescriptorHeap* const heaps[] = {
				s_ResourceDescriptorHeap->GetHeap()
			};
			cmdList->SetDescriptorHeaps(1, heaps);
			cmdList->SetPipelineState(shader->GetPipelineState());
			cmdList->SetComputeRootSignature(shader->GetRootSignature());
			
			cmdList->SetComputeRootDescriptorTable(0, envIrradiance->UAVAllocation.GPUHandle);
			cmdList->SetComputeRootDescriptorTable(1, srvAllocation.GPUHandle);

			uint32_t threadsX = envIrradiance->GetWidth() / IrrmapSize;
			uint32_t threadsY = envIrradiance->GetHeight() / IrrmapSize;
			uint32_t threadsZ = envIrradiance->GetDepth();
			cmdList->Dispatch(threadsX, threadsY, threadsZ);

			envIrradiance->Transition(batch, D3D12_RESOURCE_STATE_COMMON);

			PIXEndEvent(cmdList.Get());
			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();

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

		Profiler::GlobalProfiler.Initialize();

		D3D12Renderer::ShaderLibrary = new Hazel::ShaderLibrary(D3D12Renderer::FrameLatency);
		D3D12Renderer::TextureLibrary = new Hazel::TextureLibrary();
		D3D12Renderer::TilePool = new Hazel::D3D12TilePool();
#pragma region Renderer Implementations
		// Add the two renderer implementations
		{
			s_AvailableRenderers.resize(RendererType_Count);
			s_AvailableRenderers[RendererType_Forward] = new D3D12ForwardRenderer();
			s_AvailableRenderers[RendererType_Forward]->ImplOnInit();
			s_AvailableRenderers[RendererType_TextureSpace] = new DecoupledRenderer();
			s_AvailableRenderers[RendererType_TextureSpace]->ImplOnInit();
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
			D3D12Renderer::ShaderLibrary->Add(shader);
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
			D3D12Renderer::ShaderLibrary->Add(shader);
		}

		// Equirectangular to Cube
		{
			CD3DX12_PIPELINE_STATE_STREAM stream;

			auto shader = CreateRef<D3D12Shader>("assets/shaders/Equirectangular2Cube.hlsl", stream, ShaderType::Compute);
			HZ_CORE_ASSERT(shader, "Shader was null");
			D3D12Renderer::ShaderLibrary->Add(shader);
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
				D3D12Renderer::ShaderLibrary->Add(shader);
			}
		}

		{
			CD3DX12_PIPELINE_STATE_STREAM stream;
		}

		D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
		auto cmdList = batch.Begin();

		// Create SPBRDF_LUT
		{
			auto shader = ShaderLibrary->GetAs<D3D12Shader>("SPBRDF_LUT");

			D3D12Texture::TextureCreationOptions opts;
			opts.Name = "spbrdf";
			opts.Width = 256;
			opts.Height = 256;
			opts.Depth = 1;
			opts.Format = DXGI_FORMAT_R16G16_FLOAT;
			opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			opts.MipLevels = 1;

			auto tex = D3D12Texture2D::CreateCommittedTexture(batch, opts);
			tex->Transition(batch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			CreateUAV(std::static_pointer_cast<D3D12Texture>(tex), 0);

			cmdList->SetComputeRootSignature(shader->GetRootSignature());
			cmdList->SetPipelineState(shader->GetPipelineState());

			cmdList->SetPipelineState(shader->GetPipelineState());
			ID3D12DescriptorHeap* heaps[] = {
			   s_ResourceDescriptorHeap->GetHeap()
			};
			cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
			cmdList->SetComputeRootDescriptorTable(0, tex->UAVAllocation.GPUHandle);
			cmdList->Dispatch(tex->GetWidth() / 32, tex->GetHeight() / 32, 1);

			tex->Transition(batch, D3D12_RESOURCE_STATE_COMMON);
			TextureLibrary->Add(tex);
		}

		std::vector<QuadVertex> verts(4);
		verts[0] = { { -1.0f , -1.0f, 0.1f } };
		verts[1] = { {  1.0f , -1.0f, 0.1f } };
		verts[2] = { {  1.0f ,  1.0f, 0.1f } };
		verts[3] = { { -1.0f ,  1.0f, 0.1f } };

		std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

		s_FullscreenQuadVB = new D3D12VertexBuffer(batch, (float*)verts.data(), verts.size() * sizeof(QuadVertex));
		s_FullscreenQuadIB = new D3D12IndexBuffer(batch, indices.data(), indices.size());

		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
		//Context->DeviceResources->CommandAllocator->Reset();

		auto t = TextureLibrary->Get(std::string("spbrdf"));
		s_ResourceDescriptorHeap->Release(t->UAVAllocation);


#pragma endregion

#pragma Frame Buffers
		{
			s_Framebuffers.resize(D3D12Renderer::FrameLatency);
			D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
			batch.Begin();
			CreateFrameBuffers(batch);

			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
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
		
		s_ImGuiAllocation = s_ResourceDescriptorHeap->Allocate(1);
		s_CommonData.StaticResources++;

	}

	void D3D12Renderer::Shutdown()
	{
		Context->WaitForGpu();
        s_TransparentObjects.clear();
        s_OpaqueObjects.clear();
        s_DecoupledObjects.clear();
		s_LightsBuffer.release();
		delete s_FullscreenQuadVB;
		delete s_FullscreenQuadIB;

		delete Context;
		delete ShaderLibrary;
		delete TextureLibrary;
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

	void D3D12Renderer::CreateFrameBuffers()
	{
	}

	void D3D12Renderer::CreateFrameBuffers(D3D12ResourceBatch& batch)
	{
		std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
		auto cmdList = batch.GetCommandList();
		for (int i = 0; i < s_Framebuffers.size(); i++)
		{
			FrameBufferOptions opts;
			opts.Width = Context->Viewport.Width;
			opts.Height = Context->Viewport.Height;
			opts.ColourFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			opts.DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			opts.Name = "Framebuffer #" + std::to_string(i);
			s_Framebuffers[i] = FrameBuffer::Create(batch, opts);
			auto fb = s_Framebuffers[i];

			fb->RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
			CreateRTV(std::static_pointer_cast<D3D12Texture>(fb->ColorResource), fb->RTVAllocation, 0);
			s_CommonData.StaticRenderTargets++;

			fb->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
			CreateSRV(std::static_pointer_cast<D3D12Texture>(fb->ColorResource),
				fb->SRVAllocation, 0, 1);
			s_CommonData.StaticResources++;

			fb->DSVAllocation = s_DepthStencilDescriptorHeap->Allocate(1);
			CreateDSV(std::static_pointer_cast<D3D12Texture>(fb->DepthStensilResource), fb->DSVAllocation);

			fb->ColorResource->Transition(batch, D3D12_RESOURCE_STATE_RENDER_TARGET);
			fb->DepthStensilResource->Transition(batch, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			/*barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				fb->ColorResource->GetResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			));
			fb->ColorResource->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

			barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				fb->DepthStensilResource->GetResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_DEPTH_WRITE
			));
			fb->DepthStensilResource->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);*/
		}
		//cmdList->ResourceBarrier(barriers.size(), barriers.data());
	}

    void D3D12Renderer::ImplRenderSubmitted()
    {

    }

    void D3D12Renderer::ImplOnInit()
    {

    }

    void D3D12Renderer::ImplSubmit(Ref<GameObject> gameObject)
    {

    }

    void D3D12Renderer::ImplSubmit(D3D12ResourceBatch& batch, Ref<GameObject> gameObject)
    {

    }

    void D3D12Renderer::CreateUAV(Ref<D3D12Texture> texture, uint32_t mip)
	{
		if (!texture->UAVAllocation.Allocated)
		{
			texture->UAVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		}
		
		CreateUAV(texture, texture->UAVAllocation, mip);
	}

	void D3D12Renderer::CreateUAV(Ref<D3D12Texture> texture, HeapAllocationDescription& description, uint32_t mip)
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

	void D3D12Renderer::CreateSRV(Ref<D3D12Texture> texture, uint32_t mostDetailedMip, uint32_t mips, bool forceArray)
	{
		if (!texture->SRVAllocation.Allocated)
		{
			texture->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
		}
		
		CreateSRV(texture, texture->SRVAllocation, mostDetailedMip, mips, forceArray);
	}

	void D3D12Renderer::CreateSRV(Ref<D3D12Texture> texture, HeapAllocationDescription& description, uint32_t mostDetailedMip, uint32_t mips, bool forceArray)
	{
		auto desc = texture->GetResource()->GetDesc();

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
			texture->GetResource(),
			&srvDesc,
			description.CPUHandle
		);
	}

	void D3D12Renderer::CreateRTV(Ref<D3D12Texture> texture, uint32_t mip)
	{
		if (!texture->RTVAllocation.Allocated)
		{
			texture->RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
		}

		CreateRTV(texture, texture->RTVAllocation, mip);
	}

	void D3D12Renderer::CreateRTV(Ref<D3D12Texture> texture, HeapAllocationDescription& description, uint32_t mip)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = texture->GetFormat();
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = mip;

		Context->DeviceResources->Device->CreateRenderTargetView(
			texture->GetResource(),
			&rtvDesc,
			description.CPUHandle
		);
	}

	void D3D12Renderer::CreateDSV(Ref<D3D12Texture> texture, HeapAllocationDescription& description)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = texture->GetFormat();
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		Context->DeviceResources->Device->CreateDepthStencilView(texture->GetResource(), &dsvDesc, description.CPUHandle);
	}

	void D3D12Renderer::GenerateMips(Ref<D3D12Texture> texture, uint32_t mostDetailedMip)
	{
		HeapValidationMark mark(s_ResourceDescriptorHeap);

		auto srcDesc = texture->GetResource()->GetDesc();
		uint32_t remainingMips = (srcDesc.MipLevels > mostDetailedMip) ? srcDesc.MipLevels - mostDetailedMip - 1 : srcDesc.MipLevels - 1;

		HeapAllocationDescription srcAllocation = s_ResourceDescriptorHeap->Allocate(1);

		HZ_CORE_ASSERT(srcAllocation.Allocated, "Run out of heap space. Something is not releasing properly");
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

			Context->DeviceResources->Device->CreateShaderResourceView(
				texture->GetResource(),
				&srvDesc,
				srcAllocation.CPUHandle
			);
		}
		

		D3D12ResourceBatch batch(Context->DeviceResources->Device);

		Ref<D3D12Shader> shader = nullptr;
		if (srcDesc.DepthOrArraySize > 1)
		{
			shader = ShaderLibrary->GetAs<D3D12Shader>("MipGeneration-Array");
		}
		else
		{
			shader = ShaderLibrary->GetAs<D3D12Shader>("MipGeneration-Linear");
		}

		auto cmdList = batch.Begin();
		PIXBeginEvent(cmdList.Get(), PIX_COLOR(255, 0, 255), "Generate Mips: %s", texture->GetIdentifier().c_str());

		texture->Transition(batch, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cmdList->SetComputeRootSignature(shader->GetRootSignature());
		cmdList->SetPipelineState(shader->GetPipelineState());
		 ID3D12DescriptorHeap * heaps[] = {
			s_ResourceDescriptorHeap->GetHeap()
		};
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

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

			cmdList->SetComputeRoot32BitConstants(0, sizeof(passData) / sizeof(uint32_t), &passData, 0);
			cmdList->SetComputeRootDescriptorTable(1, srcAllocation.GPUHandle);
            cmdList->SetComputeRootDescriptorTable(2, heapAllocations[allocationCounter + 0].GPUHandle);
            cmdList->SetComputeRootDescriptorTable(3, heapAllocations[allocationCounter + 1].GPUHandle);
            cmdList->SetComputeRootDescriptorTable(4, heapAllocations[allocationCounter + 2].GPUHandle);
            cmdList->SetComputeRootDescriptorTable(5, heapAllocations[allocationCounter + 3].GPUHandle);

			auto x_count = D3D12::RoundToMultiple(dstWidth, 8);
			auto y_count = D3D12::RoundToMultiple(dstHeight, 8);
			cmdList->Dispatch(x_count, y_count, srcDesc.DepthOrArraySize);
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(texture->GetResource()));
			srcMip += mipCount;
			allocationCounter += 4;
		}

		PIXEndEvent(cmdList.Get());
		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();

		for (auto& a : heapAllocations)
		{
			s_ResourceDescriptorHeap->Release(a);
		}
		s_ResourceDescriptorHeap->Release(srcAllocation);
	}

	void D3D12Renderer::ClearUAV(ID3D12GraphicsCommandList* cmdlist, Ref<D3D12FeedbackMap>& resource, uint32_t value)
	{
		auto shader = ShaderLibrary->GetAs<D3D12Shader>("ClearUAV");


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
		if (s_DecoupledObjects.size() == 0)
			return;

		//Context->DeviceResources->CommandAllocator->Reset();
		// Readback
		{
			D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
			auto cmdlist = batch.Begin();
			GPUProfileBlock gpuBlock(cmdlist.Get(), "Virtual Texture Update");
			for (auto obj : s_DecoupledObjects)
			{
				auto tex = obj->DecoupledComponent.VirtualTexture;
				GPUProfileBlock block(cmdlist.Get(), "Update : " + tex->GetIdentifier());
				auto feedback = tex->GetFeedbackMap();
				feedback->Update(batch.GetCommandList().Get());
				auto mips = tex->ExtractMipsUsed();
                TilePool->MapTexture(batch, tex, Context->DeviceResources->CommandQueue);
                auto mip = mips.FinestMip >= tex->GetMipLevels() ? tex->GetMipLevels() - 1 : mips.FinestMip;

                CreateSRV(std::static_pointer_cast<D3D12Texture>(tex), mip);
				batch.TrackBlock(block);
			}
			batch.TrackBlock(gpuBlock);
			batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
		}
	}

	void D3D12Renderer::RenderVirtualTextures()
	{
		if (s_DecoupledObjects.size() == 0)
			return;

        auto renderer = dynamic_cast<DecoupledRenderer*>(s_AvailableRenderers[RendererType_TextureSpace]);
		renderer->ImplRenderVirtualTextures();
		renderer->ImplDilateVirtualTextures();
	}

	void D3D12Renderer::GenerateVirtualMips()
	{
		HeapValidationMark mark(s_ResourceDescriptorHeap);
		for (auto obj : s_DecoupledObjects)
		{
			auto vtex = obj->DecoupledComponent.VirtualTexture;
			auto tex = std::static_pointer_cast<D3D12Texture>(vtex);
			auto mips = vtex->GetMipsUsed();
			GenerateMips(tex, mips.FinestMip);
		}
	}

	void D3D12Renderer::ClearVirtualMaps()
	{
		auto shader = ShaderLibrary->GetAs<D3D12Shader>(ShaderNameClearUAV);
		ID3D12DescriptorHeap* const heaps[] = {
			s_ResourceDescriptorHeap->GetHeap()
		};

		D3D12ResourceBatch batch(Context->DeviceResources->Device, Context->DeviceResources->CommandAllocator);
		auto cmdlist = batch.Begin();

		PIXBeginEvent(cmdlist.Get(), PIX_COLOR(1, 0, 1), "Clear feedback");

		cmdlist->SetPipelineState(shader->GetPipelineState());
		cmdlist->SetComputeRootSignature(shader->GetRootSignature());
		cmdlist->SetDescriptorHeaps(_countof(heaps), heaps);
		for (auto obj : s_DecoupledObjects)
		{
			auto tex = obj->DecoupledComponent.VirtualTexture;
			auto mips = tex->GetMipLevels();
			auto fb = tex->GetFeedbackMap();
			auto dims = fb->GetDimensions();

			if (mips == 1)
			{
				__debugbreak();
			}

			cmdlist->SetComputeRoot32BitConstants(0, 1, &mips, 0);
			cmdlist->SetComputeRootDescriptorTable(1, fb->UAVAllocation.GPUHandle);

			auto dispatch_count = Hazel::D3D12::RoundToMultiple(dims.x * dims.y, 64);

			cmdlist->Dispatch(dispatch_count, 1, 1);
		}
		PIXEndEvent(cmdlist.Get());
		batch.End(Context->DeviceResources->CommandQueue.Get()).wait();
	}
	

}


