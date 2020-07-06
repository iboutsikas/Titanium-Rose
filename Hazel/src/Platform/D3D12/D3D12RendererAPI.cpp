#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Platform/D3D12/D3D12RendererAPI.h"
#include "Platform/D3D12/D3D12Context.h"


#include "glm/gtc/type_ptr.hpp"

namespace Hazel {

	void D3D12RendererAPI::Init()
	{

	}

	void D3D12RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		HZ_CORE_ASSERT(false, "This shouldn't be called");
	}

	void D3D12RendererAPI::SetClearColor(const glm::vec4& color)
	{
#if 0
		m_ClearColor = color;
#endif
		HZ_CORE_ASSERT(false, "This shouldn't be called");

	}

	void D3D12RendererAPI::Clear()
	{
#if 0
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
#endif
		HZ_CORE_ASSERT(false, "This shouldn't be called");

	}

	void D3D12RendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
	}

	void D3D12RendererAPI::BeginFrame()
	{
#if 0
		Context->NewFrame();
#endif
		HZ_CORE_ASSERT(false, "This shouldn't be called");

	}

	void D3D12RendererAPI::EndFrame()
	{
#if 0
		HZ_PROFILE_FUNCTION();
		Context->SwapBuffers();
#endif
		HZ_CORE_ASSERT(false, "This shouldn't be called");

	}
}


