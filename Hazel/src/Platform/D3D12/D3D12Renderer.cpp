#include "hzpch.h"
#include "D3D12Renderer.h"

#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12ForwardRenderer.h"


#define MAX_RESOURCES       100
#define MAX_RENDERTARGETS   30

namespace Hazel {
	D3D12Context*	D3D12Renderer::Context;
	ShaderLibrary*	D3D12Renderer::ShaderLibrary;
	TextureLibrary* D3D12Renderer::TextureLibrary;

	D3D12DescriptorHeap* D3D12Renderer::s_ResourceDescriptorHeap;
	D3D12DescriptorHeap* D3D12Renderer::s_RenderTargetDescriptorHeap;

	D3D12Renderer::CommonData D3D12Renderer::s_CommonData;

	Scope<D3D12UploadBuffer<D3D12Renderer::RendererLight>> D3D12Renderer::s_LightsBuffer;
	HeapAllocationDescription D3D12Renderer::s_LightsBufferAllocation;

	D3D12_INPUT_ELEMENT_DESC D3D12Renderer::s_InputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Hazel::Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Hazel::Vertex, Tangent), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

	void D3D12Renderer::AddStaticResource(Ref<D3D12Texture2D> texture)
	{
		texture->DescriptorAllocation = s_ResourceDescriptorHeap->Allocate(1);
		HZ_CORE_ASSERT(texture->DescriptorAllocation.Allocated, "Could not allocate space on the resource heap");
		D3D12Renderer::Context->DeviceResources->Device->CreateShaderResourceView(
			texture->GetResource(),
			nullptr,
			texture->DescriptorAllocation.CPUHandle
		);
	}

	void D3D12Renderer::AddStaticRenderTarget(Ref<D3D12Texture2D> texture)
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

	void D3D12Renderer::Init()
	{
		auto& wnd = Application::Get().GetWindow();
		Context = new D3D12Context();
		Context->Init(&wnd);

		D3D12Renderer::ShaderLibrary = new Hazel::ShaderLibrary(3);
		D3D12Renderer::TextureLibrary = new Hazel::TextureLibrary();

		s_AvailableRenderers.resize(RendererType_Count);
		s_AvailableRenderers[RendererType_Forward] = new D3D12ForwardRenderer();
		s_AvailableRenderers[RendererType_Forward]->ImplOnInit();
		// s_AvailableRenderers[RendererType_TextureSpace] = new Texture space renderer
		s_CurrentRenderer = s_AvailableRenderers[RendererType_Forward];

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


		HZ_CORE_ASSERT((sizeof(RendererLight) % 16) == 0, "The size of the light struct should be 128-bit aligned");

		s_LightsBuffer = CreateScope<D3D12UploadBuffer<RendererLight>>(MaxSupportedLights, false);
		s_LightsBuffer->Resource()->SetName(L"Lights buffer");

		s_LightsBufferAllocation = s_ResourceDescriptorHeap->Allocate(1);

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

	void D3D12Renderer::Shutdown()
	{
		s_LightsBuffer.release();

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

