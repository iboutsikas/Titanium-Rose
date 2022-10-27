#pragma once
#include "TitaniumRose/Core/Window.h"
#include "TitaniumRose/Core/Core.h"

#include "Platform/D3D12/D3D12DeviceResources.h"
#include "Platform/D3D12/D3D12FrameResource.h"


#include <vector>
namespace Roses{

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

		void CreateSwapChain(ID3D12CommandQueue* commandQueue);
		
		void CreateBackBuffers();
		void ResetBackbuffers();
		void WaitForGpu();
		void ResizeSwapChain();
		void NewFrame();

		D3D12FrameResource* CurrentFrameResource;

		Scope<D3D12DeviceResources> DeviceResources;
		std::vector<Scope<D3D12FrameResource>> FrameResources;

		inline HWND GetNativeHandle() { return m_NativeHandle; }
		//inline UINT GetRTVDescriptorSize() { return m_RTVDescriptorSize; }
		//inline UINT GetSRVDescriptorSize() { return m_SRVDescriptorSize; }
		inline UINT	GetCurrentFrameIndex() { return m_CurrentBackbufferIndex; }

		D3D12_VIEWPORT Viewport;
		D3D12_RECT	ScissorRect;
		ColorBuffer& GetCurrentBackBuffer() { return DeviceResources->BackBuffers[m_CurrentBackbufferIndex]; }
		Capabilities RendererCapabilities;
	private:
		Window* m_Window;
		HWND m_NativeHandle;
		bool m_TearingSupported;
		bool m_VSyncEnabled;
		UINT m_CurrentBackbufferIndex;

		uint64_t  m_FenceValue = 0;
		
		void PerformInitializationTransitions();
		void NextFrameResource();
		void BuildFrameResources();

		friend class D3D12RendererAPI;
		friend class D3D12Renderer;
	};
}
