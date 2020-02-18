#include "hzpch.h"
#include "Hazel/Renderer/RenderCommand.h"

namespace Hazel {

	Scope<RendererAPI> RenderCommand::s_RendererAPI;

	void RenderCommand::Init()
	{
		HZ_CORE_ASSERT(!s_RendererAPI, "RendererAPI has already been initialized");
		s_RendererAPI = RendererAPI::Create();
		s_RendererAPI->Init();
	}

}