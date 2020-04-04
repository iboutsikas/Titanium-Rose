#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Renderer/Renderer.h"

#include "Platform/D3D12/ImGui/D3D12ImGuiLayer.h"

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
			auto ctx = Application::Get().GetWindow().GetContext();
			return new D3D12ImGuiLayer(static_cast<D3D12Context*>(ctx));
		}
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}