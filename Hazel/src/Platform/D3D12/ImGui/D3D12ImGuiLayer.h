#pragma once

#include "Hazel/ImGui/ImGuiLayer.h"

#include "Platform/D3D12/D3D12Context.h"

namespace Hazel {

	class D3D12ImGuiLayer : public ImGuiLayer {
		
	public:
		D3D12ImGuiLayer(D3D12Context* context);

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void Begin() override;
		virtual void End() override;

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		D3D12Context* m_Context;
	};
}