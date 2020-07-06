#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Renderer/Renderer.h"

#include "Platform/D3D12/ImGui/D3D12ImGuiLayer.h"
#include "Platform/D3D12/D3D12Renderer.h"
namespace Hazel {

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
		case RendererAPI::API::D3D12: {
			return new D3D12ImGuiLayer(D3D12Renderer::Context);
		}
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}