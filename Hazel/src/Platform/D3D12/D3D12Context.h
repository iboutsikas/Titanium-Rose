#pragma once
#include "Hazel/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Hazel{
	class D3D12Context : public GraphicsContext
	{
	public:
		D3D12Context(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;
		HWND m_NativeHandle;
	};
}

