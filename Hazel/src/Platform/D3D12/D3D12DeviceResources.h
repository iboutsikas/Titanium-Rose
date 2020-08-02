#pragma once

#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Core/Window.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Helpers.h"

#include <vector>
// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
// D3D12 extension library.
#include "d3dx12.h"



namespace Hazel {

    struct SwapChainCreationOptions {
        UINT Width;
        UINT Height;
        bool TearingSupported;
        UINT BufferCount;
        HWND Handle;
    };

    class D3D12DeviceResources {
    public:

        D3D12DeviceResources(UINT bufferCount);

        TComPtr<ID3D12Device2>                  Device;
        TComPtr<ID3D12CommandQueue>             CommandQueue;
        TComPtr<IDXGISwapChain4>                SwapChain;
        std::vector<TComPtr<ID3D12Resource>>    BackBuffers;
        TComPtr<ID3D12Resource>                 DepthStencilBuffer;
        TComPtr<ID3D12GraphicsCommandList>      CommandList;
        TComPtr<ID3D12CommandAllocator>         CommandAllocator;
        TComPtr<ID3D12DescriptorHeap>           RTVDescriptorHeap;
        //TComPtr<ID3D12DescriptorHeap>           SRVDescriptorHeap;
        //TComPtr<ID3D12DescriptorHeap>           DSVDescriptorHeap;
        TComPtr<ID3D12Fence>                    Fence;

        int SwapChainBufferCount;

        void EnableDebugLayer();

        TComPtr<IDXGIAdapter4> GetAdapter(bool useWarp, D3D12::VendorID preferedVendor = D3D12::VendorID::NVIDIA);

        TComPtr<ID3D12Device2> CreateDevice(TComPtr<IDXGIAdapter4> adapter);

        TComPtr<ID3D12CommandQueue> CreateCommandQueue(
            TComPtr<ID3D12Device2> device, 
            D3D12_COMMAND_LIST_TYPE type);

        TComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
            TComPtr<ID3D12Device2> device,
            D3D12_COMMAND_LIST_TYPE type);

        TComPtr<ID3D12GraphicsCommandList> CreateCommandList(
            TComPtr<ID3D12Device2> device,
            TComPtr<ID3D12CommandAllocator> commandAllocator,
            D3D12_COMMAND_LIST_TYPE type,
            bool closeList = true);

        TComPtr<IDXGISwapChain4> CreateSwapChain(
            SwapChainCreationOptions& opts,
            TComPtr<ID3D12CommandQueue> commandQueue);

        TComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
            TComPtr<ID3D12Device2> device,
            D3D12_DESCRIPTOR_HEAP_TYPE type, 
            uint32_t numDescriptors, 
            D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        TComPtr<ID3D12Fence> CreateFence(TComPtr<ID3D12Device2> device);

        uint64_t Signal(
            TComPtr<ID3D12CommandQueue> commandQueue, 
            TComPtr<ID3D12Fence> fence, 
            uint64_t fenceValue);

        void WaitForFenceValue(
            TComPtr<ID3D12Fence> fence, 
            uint64_t fenceValue,
            UINT duration = INFINITE);
    };
}