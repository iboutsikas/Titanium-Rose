#include "hzpch.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/ComPtr.h"

// Need these to expose the HWND from GLFW. Too much work to
// re-write a new win32 only window.
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <string>
#include "Hazel/Core/Log.h"

#define NUM_FRAMES 3

using namespace Hazel::D3D12;

std::string static inline VendorIDToString(VendorID id) {
	switch (id) {
	case VendorID::AMD:       return "AMD";
	case VendorID::NVIDIA:    return "NVIDIA Corporation";
	case VendorID::INTEL:     return "Intel";
	default: return "Unknown Vendor ID";
	}
}

bool CheckTearingSupport()
{
	bool allowTearing = false;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	Hazel::TComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		Hazel::TComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing))))
			{
				allowTearing = false;
			}
		}
	}

	return allowTearing == true;
}

namespace Hazel {

	D3D12Context::D3D12Context(Window* window)
		: m_Window(window), m_NativeHandle(nullptr)
	{
		HZ_CORE_ASSERT(window, "Window handle is null!");
		m_NativeHandle = glfwGetWin32Window(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()));
		HZ_CORE_ASSERT(m_NativeHandle, "D3D12Context m_NativeHandle is null!");
		//DeviceResources = CreateScope<D3D12DeviceResources>(NUM_FRAMES);
	}

	void D3D12Context::Init()
	{
//		HZ_PROFILE_FUNCTION();
//
//		
//		auto width = m_Window->GetWidth();
//		auto height = m_Window->GetHeight();
//		m_TearingSupported = CheckTearingSupport();
//
//#ifdef HZ_DEBUG
//		DeviceResources->EnableDebugLayer();
//#endif
//		// The device
//		TComPtr<IDXGIAdapter4> theAdapter = DeviceResources->GetAdapter(false);
//		DeviceResources->Device = DeviceResources->CreateDevice(theAdapter);
//		NAME_D3D12_OBJECT(DeviceResources->Device);
//
//		BuildFrameResources();
//
//		HZ_CORE_INFO("OpenGL Info:");
//		HZ_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
//		HZ_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
//		HZ_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));
	}

	void D3D12Context::SwapBuffers()
	{
	}
	void D3D12Context::SetVSync(bool enable)
	{
	}
	void D3D12Context::PerformInitializationTransitions()
	{
	}
	void D3D12Context::NextFrameResource()
	{
	}
	void D3D12Context::BuildFrameResources()
	{
	}
}