#include "hzpch.h"
#include "Hazel/Core/Window.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/Renderer.h"

#include "Platform/D3D12/D3D12Context.h"

namespace Hazel {

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
		Window* wnd = static_cast<Window*>(window);

		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
			/*case RendererAPI::API::D3D12:	return CreateScope<D3D12Context>(wnd);*/
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}