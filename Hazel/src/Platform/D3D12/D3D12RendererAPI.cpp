#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Platform/D3D12/D3D12RendererAPI.h"
#include "Platform/D3D12/D3D12Context.h"


#include "glm/gtc/type_ptr.hpp"

namespace Hazel {

	void D3D12RendererAPI::Init()
	{
		Context = static_cast<D3D12Context*>(
			Application::Get()
			.GetWindow()
			.GetContext()
		);
	}

	void D3D12RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
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

	void D3D12RendererAPI::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor = color;
	}

	void D3D12RendererAPI::Clear()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv = Context->CurrentBackBufferView();

		Context
			->DeviceResources
			->CommandList
			->ClearRenderTargetView(rtv, glm::value_ptr(m_ClearColor), 0, nullptr);
		
		Context
			->DeviceResources
			->CommandList
			->ClearDepthStencilView(
				Context->DepthStencilView(), 
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
				1.0f, 0, 0, nullptr
			);
	}

	void D3D12RendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
	}

	void D3D12RendererAPI::BeginFrame()
	{
		Context->NewFrame();
	}

	void D3D12RendererAPI::EndFrame()
	{
		Context->SwapBuffers();
	}
}


