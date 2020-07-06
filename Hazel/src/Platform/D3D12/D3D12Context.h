#pragma once
#include "Hazel/Core/Window.h"
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/GraphicsContext.h"

#include "Platform/D3D12/D3D12DeviceResources.h"
#include "Platform/D3D12/D3D12FrameResource.h"


#include <vector>
namespace Hazel{
	class D3D12Context
	{
	public:
		D3D12Context();

		void Init(Window* window);
		void SwapBuffers();
		void SetVSync(bool enable);
		
		void CreateRenderTargetViews();
		void CleanupRenderTargetViews();
		void CreateDepthStencil();
		void Flush();
		void ResizeSwapChain();
		void NewFrame();
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
		D3D12FrameResource* m_CurrentFrameResource;

		Scope<D3D12DeviceResources> DeviceResources;
		std::vector<Scope<D3D12FrameResource>> FrameResources;
		inline HWND GetNativeHandle() { return m_NativeHandle; }
		inline UINT GetRTVDescriptorSize() { return m_RTVDescriptorSize; }
		inline UINT GetSRVDescriptorSize() { return m_SRVDescriptorSize; }
		inline bool HaveSamplerFeedback() const { return m_SamplerFeedbackSupported; }
		D3D12_VIEWPORT m_Viewport;
		D3D12_RECT	m_ScissorRect;
		TComPtr<ID3D12Resource> GetCurrentBackBuffer() { return DeviceResources->BackBuffers[m_CurrentBackbufferIndex]; }

	private:
		Window* m_Window;
		HWND m_NativeHandle;
		bool m_TearingSupported;
		bool m_VSyncEnabled;
		bool m_SamplerFeedbackSupported;
		UINT m_CurrentBackbufferIndex;
		UINT m_RTVDescriptorSize;
		UINT m_SRVDescriptorSize;
		uint64_t  m_FenceValue = 0;
		
		void PerformInitializationTransitions();
		void NextFrameResource();
		void BuildFrameResources();

		friend class D3D12RendererAPI;
	};
}

