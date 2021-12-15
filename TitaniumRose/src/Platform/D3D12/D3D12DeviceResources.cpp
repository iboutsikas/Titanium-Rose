#include "trpch.h"
#include "D3D12DeviceResources.h"
#include "D3D12Helpers.h"

namespace Roses {
    D3D12DeviceResources::D3D12DeviceResources(UINT bufferCount)
    {
        if (bufferCount < 2) {
            HZ_CORE_ERROR("Buffer count cannot be less than 2. Was give {0}", bufferCount);
            bufferCount = 2;
        }

        SwapChainBufferCount = bufferCount;
        BackBuffers.resize(SwapChainBufferCount);
    }

    D3D12DeviceResources::~D3D12DeviceResources()
    {
    }

    void D3D12DeviceResources::EnableDebugLayer()
    {
#if defined(HZ_DEBUG) && !defined(HZ_NO_D3D12_DEBUG_LAYER)
        TComPtr<ID3D12Debug> debugInterface;
        D3D12::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
#endif
    }
    
    TComPtr<IDXGIAdapter4> D3D12DeviceResources::GetAdapter(bool useWarp, D3D12::VendorID preferedVendor)
    {
        TComPtr<IDXGIFactory4> dxgiFactory;
        UINT factoryFlags = 0;

#if defined(HZ_DEBUG) && !defined(HZ_NO_D3D12_DEBUG_LAYER)
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        D3D12::ThrowIfFailed(CreateDXGIFactory2(
            factoryFlags,
            IID_PPV_ARGS(&dxgiFactory)
        ));

        TComPtr<IDXGIAdapter1> dxgiAdapter1;
        TComPtr<IDXGIAdapter4> dxgiAdapter4;

        if (useWarp)
        {
            D3D12::ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
            D3D12::ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
        }
        else
        {
            // We grab the adapter with the highest VRAM. It "should" be the most performant one.
            SIZE_T maxDedicatedVideoMemory = 0;
            for (UINT i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(i, &dxgiAdapter1); ++i)
            {
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

                if (dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory) {
                    D3D12::ThrowIfFailed(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device2), nullptr));
                    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                    D3D12::ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
                }
            }
        }
        return dxgiAdapter4;
    }

    TComPtr<ID3D12Device2> D3D12DeviceResources::CreateDevice(TComPtr<IDXGIAdapter4> adapter)
    {
        TComPtr<ID3D12Device2> d3d12Device2;
        D3D12::ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
        
#if defined(HZ_DEBUG)
        // Add some message suppression in debug mode
        TComPtr<ID3D12InfoQueue> pInfoQueue;
        if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
        {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

            // Suppress whole categories of messages
            //D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] =
            {
                D3D12_MESSAGE_SEVERITY_INFO
            };

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
                D3D12_MESSAGE_ID_RESOLVE_QUERY_INVALID_QUERY_STATE,             // For the marking of invalid timestamps in our profiler
                (D3D12_MESSAGE_ID)1008                                          // RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
            };

            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            //NewFilter.DenyList.NumCategories = _countof(Categories);
            //NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs = _countof(DenyIds);
            NewFilter.DenyList.pIDList = DenyIds;

            D3D12::ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
        }
#endif

        return d3d12Device2;
    }

    TComPtr<ID3D12CommandQueue> D3D12DeviceResources::CreateCommandQueue(TComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        TComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        // NOTE: For multi adapter this needs to have 1 bit set for the adapter we wanna use.
        // also setting this to 1 would still work the exact same way for 1 GPU systems, but
        // we are going with what the documentation says now.
        desc.NodeMask = 0;

        D3D12::ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

        return d3d12CommandQueue;
    }

    TComPtr<ID3D12CommandAllocator> D3D12DeviceResources::CreateCommandAllocator(TComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        TComPtr<ID3D12CommandAllocator> commandAllocator;
        D3D12::ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

        return commandAllocator;
    }

    TComPtr<ID3D12GraphicsCommandList> D3D12DeviceResources::CreateCommandList(TComPtr<ID3D12Device2> device, TComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type, bool closeList)
    {
        TComPtr<ID3D12GraphicsCommandList> commandList;
        D3D12::ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

        if (closeList) {
            D3D12::ThrowIfFailed(commandList->Close());
        }

        return commandList;
    }

    TComPtr<IDXGISwapChain4> D3D12DeviceResources::CreateSwapChain(SwapChainCreationOptions& opts, ID3D12CommandQueue* commandQueue)
    {
        TComPtr<IDXGISwapChain4> dxgiSwapChain4;
        TComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;

#if defined(HZ_DEBUG) && !defined(HZ_NO_D3D12_DEBUG_LAYER)
        createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        D3D12::ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = opts.Width;
        swapChainDesc.Height = opts.Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc = { 1, 0 };
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = opts.BufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        // TODO: It is recommended to always allow tearing if tearing support is available.
        swapChainDesc.Flags = opts.TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        TComPtr<IDXGISwapChain1> swapChain1;
        D3D12::ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
            commandQueue,
            opts.Handle,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        D3D12::ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(opts.Handle, DXGI_MWA_NO_ALT_ENTER));

        D3D12::ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

        return dxgiSwapChain4;
    }

    TComPtr<ID3D12DescriptorHeap> D3D12DeviceResources::CreateDescriptorHeap(TComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags /*= D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
    {
        TComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = numDescriptors;
        desc.Type = type;
        desc.Flags = flags;

        D3D12::ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

        return descriptorHeap;
    }

    TComPtr<ID3D12Fence> D3D12DeviceResources::CreateFence(TComPtr<ID3D12Device2> device)
    {
        TComPtr<ID3D12Fence> fence;

        D3D12::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        return fence;
    }
#if 0
    uint64_t D3D12DeviceResources::Signal(TComPtr<ID3D12CommandQueue> commandQueue, TComPtr<ID3D12Fence> fence, uint64_t fenceValue)
    {
        uint64_t val = ++fenceValue;
        D3D12::ThrowIfFailed(commandQueue->Signal(fence.Get(), val));

        return val;
    }

    void D3D12DeviceResources::WaitForFenceValue(TComPtr<ID3D12Fence> fence, uint64_t fenceValue, UINT duration)
    {
        if (fence->GetCompletedValue() < fenceValue)
        {
            HANDLE evt = ::CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

            D3D12::ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, evt));

            ::WaitForSingleObject(evt, duration);

            ::CloseHandle(evt);
        }
    }
#endif
}