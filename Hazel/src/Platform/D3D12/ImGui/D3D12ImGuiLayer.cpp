#include "hzpch.h"

#include "Platform/D3D12/ImGui/D3D12ImGuiLayer.h"

#include "imgui.h"

#include "examples/imgui_impl_dx12.h"
#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_dx12.cpp"
#include "examples/imgui_impl_win32.cpp"

namespace Hazel {
    static LRESULT(*originalWindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;

    D3D12ImGuiLayer::D3D12ImGuiLayer(D3D12Context* context)
        : m_Context(context)
    {
        originalWindowProc = (WNDPROC)::GetWindowLongPtr(m_Context->GetNativeHandle(), GWLP_WNDPROC);
        ::SetWindowLongPtr(m_Context->GetNativeHandle(), GWLP_WNDPROC, (LONG_PTR)Hazel::D3D12ImGuiLayer::WindowProc);
    }

    void D3D12ImGuiLayer::OnAttach()
	{
        HZ_PROFILE_FUNCTION();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        // TODO: Figure out a way for ImGui to have its own heap maybe ?
		//m_SRVHeap = m_Context->DeviceResources->CreateDescriptorHeap(
  //          m_Context->DeviceResources->Device,
		//	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		//	1,
		//	D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		//);
  //      m_SRVHeap->SetName(L"ImGuiLayer SRV Heap");
		

        // Setup Platform/Renderer bindings
        ImGui_ImplWin32_Init(m_Context->GetNativeHandle());
        auto r = m_Context->DeviceResources.get();
        ImGui_ImplDX12_Init(r->Device.Get(), 
            r->BackBuffers.size(),
            DXGI_FORMAT_R8G8B8A8_UNORM, 
            r->SRVDescriptorHeap.Get(),
            r->SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            r->SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	void D3D12ImGuiLayer::OnDetach()
	{
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
	}
	void D3D12ImGuiLayer::Begin()
	{
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
	}
	void D3D12ImGuiLayer::End()
	{
        ImGuiIO& io = ImGui::GetIO();
        //Application& app = Application::Get();
        //io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());
        auto r = m_Context->DeviceResources.get();
        // Rendering
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), r->CommandList.Get());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(NULL, (void*)r->CommandList.Get());
        }
	}

    LRESULT D3D12ImGuiLayer::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;

        return ::CallWindowProc(originalWindowProc, hwnd, uMsg, wParam, lParam);
    }
}