#include "hzpch.h"

#include "Platform/D3D12/ImGui/D3D12ImGuiLayer.h"
#include "Platform/D3D12/D3D12Renderer.h"

#include "imgui.h"

#include "examples/imgui_impl_dx12.h"
#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_dx12.cpp"
#include "examples/imgui_impl_win32.cpp"

#include "WinPixEventRuntime/pix3.h"

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
        
        m_FontTexureDescription = D3D12Renderer::GetImguiAllocation();

        // Setup Platform/Renderer bindings
        ImGui_ImplWin32_Init(m_Context->GetNativeHandle());
        auto r = m_Context->DeviceResources.get();
        ImGui_ImplDX12_Init(r->Device.Get(), 
            r->BackBuffers.size(),
            DXGI_FORMAT_R8G8B8A8_UNORM, 
            D3D12Renderer::s_ResourceDescriptorHeap->GetHeap(),
            m_FontTexureDescription.CPUHandle,
            m_FontTexureDescription.GPUHandle);
	}
	void D3D12ImGuiLayer::OnDetach()
	{
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
	}
	void D3D12ImGuiLayer::Begin(GraphicsContext& context)
	{
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ColorBuffer& backbuffer = D3D12Renderer::Context->GetCurrentBackBuffer();

        context.TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        context.GetCommandList()->OMSetRenderTargets(1, &(backbuffer.GetRTV().CPUHandle), TRUE, nullptr);
	}

	void D3D12ImGuiLayer::End(GraphicsContext& context)
	{
        ImGuiIO& io = ImGui::GetIO();
        // Rendering
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.GetCommandList());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(NULL, (void*)(context.GetCommandList()));
        }
	}

    LRESULT D3D12ImGuiLayer::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return true;

        return ::CallWindowProc(originalWindowProc, hwnd, uMsg, wParam, lParam);
    }
}