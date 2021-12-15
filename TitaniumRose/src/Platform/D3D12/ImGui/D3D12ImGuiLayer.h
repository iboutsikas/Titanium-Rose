#pragma once

#include "TitaniumRose/ImGui/ImGuiLayer.h"

#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"

#include "Platform/D3D12/CommandContext.h"

namespace Roses {
	
	class D3D12ImGuiLayer : public ImGuiLayer {
		
	public:
		D3D12ImGuiLayer(D3D12Context* context);

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void Begin(GraphicsContext& context) override;
		virtual void End(GraphicsContext& context) override;

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		D3D12Context* m_Context;
		HeapAllocationDescription m_FontTexureDescription;
	};
}