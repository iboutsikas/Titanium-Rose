#include "hzpch.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/CommandQueue.h"	

// Need these to expose the HWND from GLFW. Too much work to
// re-write a new win32 only window.
#ifndef GLFW_EXPOSE_NATIVE_WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif // !1

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <string>
#include "Hazel/Core/Log.h"

#include "WinPixEventRuntime/pix3.h"
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

	D3D12Context::D3D12Context()
		: m_Window(), m_NativeHandle(nullptr), ScissorRect({0, 0, LONG_MAX, LONG_MAX})
	{
		
	}

	void D3D12Context::Init(Window* window)
	{
		HZ_PROFILE_FUNCTION();
		m_Window = window;
		HZ_CORE_ASSERT(window, "Window handle is null!");
		m_NativeHandle = glfwGetWin32Window(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()));
		HZ_CORE_ASSERT(m_NativeHandle, "D3D12Context m_NativeHandle is null!");
		DeviceResources = CreateScope<D3D12DeviceResources>(D3D12Renderer::FrameLatency);


		auto width = m_Window->GetWidth();
		auto height = m_Window->GetHeight();
		m_TearingSupported = CheckTearingSupport();

#ifdef HZ_DEBUG
		DeviceResources->EnableDebugLayer();
#endif

#pragma region The Device
		// The device
		TComPtr<IDXGIAdapter4> theAdapter = DeviceResources->GetAdapter(false, VendorID::NVIDIA);
		DeviceResources->Device = DeviceResources->CreateDevice(theAdapter);
		NAME_D3D12_OBJECT(DeviceResources->Device);
#pragma endregion

		BuildFrameResources();

#pragma region Command Objects
#if 0
		// The command queue        
		DeviceResources->CommandQueue = DeviceResources->CreateCommandQueue(
			DeviceResources->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT
		);
		NAME_D3D12_OBJECT(DeviceResources->CommandQueue);

		DeviceResources->CommandAllocator = DeviceResources->CreateCommandAllocator(
			DeviceResources->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT
		);
		NAME_D3D12_OBJECT(DeviceResources->CommandAllocator);

		DeviceResources->MainCommandList = CreateRef<D3D12CommandList>(
			DeviceResources->Device,
			D3D12_COMMAND_LIST_TYPE_DIRECT
		);
		DeviceResources->MainCommandList->SetName("DeviceResources->MainCommandList");

        DeviceResources->WorkerCommandList = CreateRef<D3D12CommandList>(
            DeviceResources->Device,
            D3D12_COMMAND_LIST_TYPE_DIRECT
        );
		DeviceResources->WorkerCommandList->SetName("DeviceResources->WorkerCommandList");

        DeviceResources->DecoupledCommandList = CreateRef<D3D12CommandList>(
            DeviceResources->Device,
            D3D12_COMMAND_LIST_TYPE_DIRECT
        );
        DeviceResources->DecoupledCommandList->SetName("DeviceResources->DecoupledCommandList");
#endif
#pragma endregion

		// Initial viewport
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = static_cast<float>(width);
		Viewport.Height = static_cast<float>(height);
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;


		DXGI_ADAPTER_DESC3 desc;
		theAdapter->GetDesc3(&desc);
		std::wstring description(desc.Description);

		RendererCapabilities.Vendor = VendorIDToString((VendorID)desc.VendorId);
		RendererCapabilities.Device = std::string(description.begin(), description.end());
		RendererCapabilities.Version = "Direct3D 12.0";

		HZ_CORE_INFO("DirectX Info:");
		HZ_CORE_INFO("  Vendor: {0}", RendererCapabilities.Vendor);
		HZ_CORE_INFO("  Renderer: {0}", RendererCapabilities.Device);
		HZ_CORE_INFO("  Version: {0}", RendererCapabilities.Version);
	}

	void D3D12Context::SwapBuffers()
	{
		UINT syncInterval = m_VSyncEnabled ? 1 : 0;
		UINT presentFlags = m_TearingSupported && !m_VSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(DeviceResources->SwapChain->Present(syncInterval, presentFlags));

		// Get new backbuffer index
		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

		// Signal the queue
		//m_FenceValue = DeviceResources->Signal(
		//	DeviceResources->CommandQueue,
		//	DeviceResources->Fence,
		//	m_FenceValue
		//);

		//// Update the resource
		//CurrentFrameResource->FenceValue = m_FenceValue;
	}

	void D3D12Context::SetVSync(bool enable)
	{
		m_VSyncEnabled = enable;
	}

	void D3D12Context::CreateSwapChain(ID3D12CommandQueue* commandQueue)
	{
        // The Swap Chain
        SwapChainCreationOptions opts = { 0 };
        opts.Width = m_Window->GetWidth();
        opts.Height = m_Window->GetHeight();
        opts.BufferCount = DeviceResources->SwapChainBufferCount;
        opts.TearingSupported = m_TearingSupported;
        opts.Handle = m_NativeHandle;

        DeviceResources->SwapChain = DeviceResources->CreateSwapChain(
            opts,
			commandQueue
        );

        m_CurrentBackbufferIndex = DeviceResources
            ->SwapChain
            ->GetCurrentBackBufferIndex();
	}

	void D3D12Context::CreateBackBuffers()
	{
		std::stringstream ss;
		for (int i = 0; i < DeviceResources->SwapChainBufferCount; ++i)
		{
			ID3D12Resource* backBuffer;

			ThrowIfFailed(DeviceResources->SwapChain
				->GetBuffer(i, IID_PPV_ARGS(&backBuffer))
			);
			ss << "Backbuffer[" << i << "]";
			DeviceResources->BackBuffers[i].CreateFromSwapChain(ss.str(), backBuffer);
			ss.str("");
		}

		// The index might not be the same or the next one. We need to ask the swap chain again
		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();
	}

	void D3D12Context::ResetBackbuffers()
	{
		for (int i = 0; i < DeviceResources->SwapChainBufferCount; i++)
		{
			DeviceResources->BackBuffers[i].Reset();
		}
	}

	void D3D12Context::WaitForGpu()
	{
#if 0
#define IDLE_COMMAND_LIST(commandList, commandQueue)\
		if (!commandList->IsClosed()) {\
			commandList->ExecuteAndWait(commandQueue);\
			commandList->ClearTracked();\
		}

		IDLE_COMMAND_LIST(DeviceResources->MainCommandList, DeviceResources->CommandQueue);
		IDLE_COMMAND_LIST(DeviceResources->WorkerCommandList, DeviceResources->CommandQueue);
		IDLE_COMMAND_LIST(DeviceResources->DecoupledCommandList, DeviceResources->CommandQueue);

#undef IDLE_COMMAND_LIST
#endif
		D3D12Renderer::CommandQueueManager.WaitForIdle();
	}
	
	void D3D12Context::ResizeSwapChain()
	{
		auto width = m_Window->GetWidth();
		auto height = m_Window->GetHeight();

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(DeviceResources->SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(DeviceResources->SwapChain->ResizeBuffers(
			DeviceResources->SwapChainBufferCount,
			width,
			height,
			swapChainDesc.BufferDesc.Format,
			swapChainDesc.Flags)
		);

		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();
		Viewport.Width = static_cast<float>(width);
		Viewport.Height = static_cast<float>(height);
	}

	void D3D12Context::NewFrame()
	{
		HZ_PROFILE_FUNCTION();
		NextFrameResource();
		// Get from resource
		auto fr = CurrentFrameResource;

		//ThrowIfFailed(DeviceResources->CommandAllocator->Reset());

		//if (DeviceResources->MainCommandList->IsClosed())
		//	DeviceResources->MainCommandList->Reset(fr->CommandAllocator);
		//if (DeviceResources->DecoupledCommandList->IsClosed())
		//	DeviceResources->DecoupledCommandList->Reset(fr->CommandAllocator2);
		//if (DeviceResources->WorkerCommandList->IsClosed())
		//	DeviceResources->WorkerCommandList->Reset(DeviceResources->CommandAllocator);
	}

	//D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::CurrentBackBufferView() const
	//{
	//	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
	//		DeviceResources->RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
	//		m_CurrentBackbufferIndex,
	//		m_RTVDescriptorSize
	//	);
	//}

	void D3D12Context::PerformInitializationTransitions()
	{
		//auto commandAllocator = DeviceResources->CommandAllocator;

		//commandAllocator->Reset();
		//DeviceResources->MainCommandList->Reset(commandAllocator);

		//// Transitions go here
		//{
		//	auto dsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		//		DeviceResources->DepthStencilBuffer.Get(),
		//		D3D12_RESOURCE_STATE_COMMON,
		//		D3D12_RESOURCE_STATE_DEPTH_WRITE
		//	);

		//	DeviceResources->MainCommandList->Get()->ResourceBarrier(1, &dsBarrier);
		//}

		//DeviceResources->MainCommandList->Execute(DeviceResources->CommandQueue);
		//WaitForGpu();
	}

	void D3D12Context::NextFrameResource()
	{
		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

		CurrentFrameResource = FrameResources[m_CurrentBackbufferIndex].get();
		// Wait until this frame resource catches up
		WaitForGpu();
		CurrentFrameResource->PrepareForNewFrame();
	}

	void D3D12Context::BuildFrameResources()
	{
		auto count = DeviceResources->SwapChainBufferCount;
		FrameResources.reserve(count);

#if HZ_DEBUG
		std::wstringstream ss;
#endif

		for (int i = 0; i < count; i++)
		{
			FrameResources.push_back(CreateScope<D3D12FrameResource>(
				DeviceResources->Device,
				1
				));
			
#if HZ_DEBUG
			ss.clear();
            ss << "FrameResources[" << i << "]->CommandAllocator";
			FrameResources[i]->CommandAllocator->SetName(ss.str().c_str());
			ss.clear();
			ss << "FrameResources[" << i << "]->CommandAllocator2";
            FrameResources[i]->CommandAllocator2->SetName(ss.str().c_str());
#endif
		}

		m_CurrentBackbufferIndex = 0;
		CurrentFrameResource = FrameResources[m_CurrentBackbufferIndex].get();
	}
}