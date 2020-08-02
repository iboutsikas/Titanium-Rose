#include "hzpch.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/ComPtr.h"

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
		DeviceResources = CreateScope<D3D12DeviceResources>(NUM_FRAMES);


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

		DeviceResources->CommandList = DeviceResources->CreateCommandList(
			DeviceResources->Device,
			DeviceResources->CommandAllocator,
			D3D12_COMMAND_LIST_TYPE_DIRECT
		);
		NAME_D3D12_OBJECT(DeviceResources->CommandList);
#pragma endregion

#pragma region The Swap Chain
		// The Swap Chain
		SwapChainCreationOptions opts = { 0 };
		opts.Width = width;
		opts.Height = height;
		opts.BufferCount = DeviceResources->SwapChainBufferCount;
		opts.TearingSupported = m_TearingSupported;
		opts.Handle = m_NativeHandle;

		DeviceResources->SwapChain = DeviceResources->CreateSwapChain(
			opts,
			DeviceResources->CommandQueue
		);

		m_CurrentBackbufferIndex = DeviceResources
			->SwapChain
			->GetCurrentBackBufferIndex();
#pragma endregion

#pragma region The Heaps
		DeviceResources->RTVDescriptorHeap = DeviceResources->CreateDescriptorHeap(
			DeviceResources->Device,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			DeviceResources->SwapChainBufferCount
		);
		NAME_D3D12_OBJECT(DeviceResources->RTVDescriptorHeap);

		m_RTVDescriptorSize = DeviceResources
			->Device
			->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	/*	DeviceResources->SRVDescriptorHeap = DeviceResources->CreateDescriptorHeap(
			DeviceResources->Device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			3,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		);
		NAME_D3D12_OBJECT(DeviceResources->SRVDescriptorHeap);

		m_SRVDescriptorSize = DeviceResources->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		DeviceResources->DSVDescriptorHeap = DeviceResources->CreateDescriptorHeap(
			DeviceResources->Device,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			1
		);
		NAME_D3D12_OBJECT(DeviceResources->DSVDescriptorHeap);*/
#pragma endregion

		CreateRenderTargetViews();
		CreateDepthStencil();

		// Initial viewport
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = static_cast<float>(width);
		Viewport.Height = static_cast<float>(height);
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;

		DeviceResources->Fence = DeviceResources->CreateFence(DeviceResources->Device);

		// PerformInitializationTransitions();

		DXGI_ADAPTER_DESC3 desc;
		theAdapter->GetDesc3(&desc);

		std::wstring description(desc.Description);
		std::string str(description.begin(), description.end());
		auto vendorString = VendorIDToString((VendorID)desc.VendorId);
		HZ_CORE_INFO("DirectX 12 Info:");
		HZ_CORE_INFO("  Vendor: {0}", vendorString);
		HZ_CORE_INFO("  Renderer: {0}", str);
		HZ_CORE_INFO("  Version: Direct3D 12.0");
	}

	void D3D12Context::SwapBuffers()
	{
		HZ_PROFILE_FUNCTION();
		auto backBuffer = DeviceResources->BackBuffers[m_CurrentBackbufferIndex];

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);

		DeviceResources->CommandList->ResourceBarrier(1, &barrier);

		ThrowIfFailed(DeviceResources->CommandList->Close());

		ID3D12CommandList* const commandLists[] = {
			DeviceResources->CommandList.Get()
		};
		DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		UINT syncInterval = m_VSyncEnabled ? 1 : 0;
		UINT presentFlags = m_TearingSupported && !m_VSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(DeviceResources->SwapChain->Present(syncInterval, presentFlags));

		// Get new backbuffer index
		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

		// Signal the queue
		m_FenceValue = DeviceResources->Signal(
			DeviceResources->CommandQueue,
			DeviceResources->Fence,
			m_FenceValue
		);

		// Update the resource
		CurrentFrameResource->FenceValue = m_FenceValue;
	}

	void D3D12Context::SetVSync(bool enable)
	{
		m_VSyncEnabled = enable;
	}

	void D3D12Context::CreateRenderTargetViews()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
			DeviceResources
			->RTVDescriptorHeap
			->GetCPUDescriptorHandleForHeapStart()
		);

		for (int i = 0; i < DeviceResources->SwapChainBufferCount; ++i)
		{
			TComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(DeviceResources->SwapChain
				->GetBuffer(i, IID_PPV_ARGS(&backBuffer))
			);

			DeviceResources->Device->CreateRenderTargetView(
				backBuffer.Get(), nullptr, rtvHandle);

			DeviceResources->BackBuffers[i] = backBuffer;
			NAME_D3D12_OBJECT_INDEXED(DeviceResources->BackBuffers, i);

			rtvHandle.Offset(1, m_RTVDescriptorSize);
		}

		// The index might not be the same or the next one. We need to ask the swap chain again
		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();
	}


	void D3D12Context::CleanupRenderTargetViews()
	{
		//ThrowIfFailed(DeviceResources->CommandList->Reset(
		//	DeviceResources->CommandAllocator.Get(),
		//	nullptr)
		//);

		for (int i = 0; i < DeviceResources->SwapChainBufferCount; i++)
		{
			DeviceResources->BackBuffers[i].Reset();
		}

		//DeviceResources->DepthStencilBuffer.Reset();
	}

	void D3D12Context::CreateDepthStencil()
	{
		//D3D12_RESOURCE_DESC desc;

		//desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		//desc.Alignment = 0;
		//desc.Width = m_Window->GetWidth();
		//desc.Height = m_Window->GetHeight();
		//desc.DepthOrArraySize = 1;
		//desc.MipLevels = 1;
		//desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		//desc.SampleDesc = { 1, 0 };
		//desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		//desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		//D3D12_CLEAR_VALUE clear;
		//clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		//clear.DepthStencil.Depth = 1.0f;
		//clear.DepthStencil.Stencil = 0;

		//ThrowIfFailed(DeviceResources->Device->CreateCommittedResource(
		//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		//	D3D12_HEAP_FLAG_NONE,
		//	&desc,
		//	D3D12_RESOURCE_STATE_COMMON,
		//	&clear,
		//	IID_PPV_ARGS(DeviceResources->DepthStencilBuffer.GetAddressOf())
		//));

		//DeviceResources->Device->CreateDepthStencilView(
		//	DeviceResources->DepthStencilBuffer.Get(),
		//	nullptr,
		//	DepthStencilView()
		//);

		//NAME_D3D12_OBJECT(DeviceResources->DepthStencilBuffer);
	}
	void D3D12Context::WaitForGpu()
	{
		m_FenceValue = DeviceResources->Signal(
			DeviceResources->CommandQueue,
			DeviceResources->Fence,
			m_FenceValue
		);

		DeviceResources->WaitForFenceValue(
			DeviceResources->Fence,
			m_FenceValue
		);
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
		auto commandAllocator = CurrentFrameResource->CommandAllocator;

		ThrowIfFailed(commandAllocator->Reset());
		ThrowIfFailed(DeviceResources->CommandList->Reset(
			commandAllocator.Get(),
			nullptr)
		);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::CurrentBackBufferView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			DeviceResources->RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			m_CurrentBackbufferIndex,
			m_RTVDescriptorSize
		);
	}

	//D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::DepthStencilView() const
	//{
	//	return DeviceResources->DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//}

	void D3D12Context::PerformInitializationTransitions()
	{
		auto commandAllocator = DeviceResources->CommandAllocator;

		commandAllocator->Reset();
		DeviceResources->CommandList->Reset(commandAllocator.Get(), nullptr);

		// Transitions go here
		{
			auto dsBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
				DeviceResources->DepthStencilBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_DEPTH_WRITE
			);

			DeviceResources->CommandList->ResourceBarrier(1, &dsBarrier);
		}

		ThrowIfFailed(DeviceResources->CommandList->Close());

		ID3D12CommandList* const commandLists[] = {
			DeviceResources->CommandList.Get()
		};
		DeviceResources->CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		WaitForGpu();
	}

	void D3D12Context::NextFrameResource()
	{
		m_CurrentBackbufferIndex = DeviceResources->SwapChain->GetCurrentBackBufferIndex();

		CurrentFrameResource = FrameResources[m_CurrentBackbufferIndex].get();

		if (CurrentFrameResource->FenceValue != 0) {
			DeviceResources->WaitForFenceValue(
				DeviceResources->Fence,
				CurrentFrameResource->FenceValue
			);
		}
	}

	void D3D12Context::BuildFrameResources()
	{
		auto count = DeviceResources->SwapChainBufferCount;
		FrameResources.reserve(count);

		for (int i = 0; i < count; i++)
		{
			FrameResources.push_back(CreateScope<D3D12FrameResource>(
				DeviceResources->Device,
				1
				));

#ifdef HZ_DEBUG
			NAME_D3D12_OBJECT_WITH_INDEX(FrameResources[i]->CommandAllocator, i);
#endif
		}
	}
}