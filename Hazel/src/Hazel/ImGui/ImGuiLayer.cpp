#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/ImGui/ImGuiLayer.h"

#include "Platform/D3D12/ImGui/D3D12ImGuiLayer.h"
#include "Platform/D3D12/D3D12Renderer.h"
namespace Hazel {

	ImGuiLayer::ImGuiLayer()
		: Layer("ImGuiLayer")
	{
	}

	ImGuiLayer* ImGuiLayer::Initialize()
	{
		return new D3D12ImGuiLayer(D3D12Renderer::Context);
	}

}