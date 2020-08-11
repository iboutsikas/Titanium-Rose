#pragma once
#include "Hazel/Core/Window.h"
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/GraphicsContext.h"

#include "Platform/D3D12/D3D12DeviceResources.h"
#include "Platform/D3D12/D3D12FrameResource.h"


#include <vector>
namespace Hazel{

	class D3D12Renderer;

	class D3D12Context
	{
	public:
        struct Capabilities
        {
            std::string Vendor;
            std::string Device;
            std::string Version;
        };
	public:
		D3D12Context();

		void Init(Window* window);
		void SwapBuffers();
		void SetVSync(bool enable);
		
		void CreateRenderTargetViews();
		void CleanupRenderTargetViews();
		void CreateDepthStencil();
		void WaitForGpu();
		void ResizeSwapChain();
		void NewFrame();
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
		//D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
		D3D12FrameResource* CurrentFrameResource;

		Scope<D3D12DeviceResources> DeviceResources;
		std::vector<Scope<D3D12FrameResource>> FrameResources;
		inline HWND GetNativeHandle() { return m_NativeHandle; }
		inline UINT GetRTVDescriptorSize() { return m_RTVDescriptorSize; }
		inline UINT GetSRVDescriptorSize() { return m_SRVDescriptorSize; }
		inline UINT	GetCurrentFrameIndex() { return m_CurrentBackbufferIndex; }

		D3D12_VIEWPORT Viewport;
		D3D12_RECT	ScissorRect;
		TComPtr<ID3D12Resource> GetCurrentBackBuffer() { return DeviceResources->BackBuffers[m_CurrentBackbufferIndex]; }
		Capabilities RendererCapabilities;
	private:
		Window* m_Window;
		HWND m_NativeHandle;
		bool m_TearingSupported;
		bool m_VSyncEnabled;
		UINT m_CurrentBackbufferIndex;
		UINT m_RTVDescriptorSize;
		UINT m_SRVDescriptorSize;
		uint64_t  m_FenceValue = 0;
		
		void PerformInitializationTransitions();
		void NextFrameResource();
		void BuildFrameResources();

		friend class D3D12RendererAPI;
		friend class D3D12Renderer;
	};
}

