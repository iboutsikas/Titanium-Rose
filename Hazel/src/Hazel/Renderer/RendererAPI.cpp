#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/D3D12/D3D12RendererAPI.h"

namespace Hazel {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

	Scope<RendererAPI> RendererAPI::Create()
	{
		switch (s_API)
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLRendererAPI>();
			case RendererAPI::API::D3D12:	return CreateScope<D3D12RendererAPI>();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}