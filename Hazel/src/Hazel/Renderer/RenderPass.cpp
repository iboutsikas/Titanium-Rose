#include "hzpch.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/Renderer.h"

#include "Platform/D3D12/D3D12RenderPass.h"
namespace Hazel {
	RenderPass::RenderPass()
	{
	}
	
	RenderPass::~RenderPass()
	{
	}
	
	Ref<RenderPass> RenderPass::Create() {
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
		//case RendererAPI::API::D3D12:	return CreateRef<D3D12RenderPass>();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
};
