#pragma once
#include "Hazel/Core/Window.h"
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/GraphicsContext.h"

#include "Platform/D3D12/D3D12DeviceResources.h"

namespace Hazel{
	class D3D12Context : public GraphicsContext
	{
	public:
		D3D12Context(Window* window);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void SetVSync(bool enable) override;
		Scope<D3D12DeviceResources> DeviceResources;

	private:
		Window* m_Window;
		HWND m_NativeHandle;
		bool m_TearingSupported;

		void PerformInitializationTransitions();
		void NextFrameResource();
		void BuildFrameResources();
	};
}

