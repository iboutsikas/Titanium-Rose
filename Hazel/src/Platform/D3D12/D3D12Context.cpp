#include "hzpch.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/ComPtr.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
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

namespace Hazel {

	D3D12Context::D3D12Context(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle), m_NativeHandle(nullptr)
	{
		HZ_CORE_ASSERT(windowHandle, "Window handle is null!");
		m_NativeHandle = glfwGetWin32Window(m_WindowHandle);
		HZ_CORE_ASSERT(m_NativeHandle, "D3D12Context m_NativeHandle is null!");
	}

	void D3D12Context::Init()
	{
	}

	void D3D12Context::SwapBuffers()
	{
	}
}