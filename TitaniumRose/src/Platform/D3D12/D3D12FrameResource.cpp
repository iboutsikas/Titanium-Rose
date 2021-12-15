#include "trpch.h"
#include "Platform/D3D12/D3D12FrameResource.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Roses {
    D3D12FrameResource::D3D12FrameResource(TComPtr<ID3D12Device> device, UINT passCount)
    {
        D3D12::ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CommandAllocator.GetAddressOf())));

        D3D12::ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CommandAllocator2.GetAddressOf())));
    }

    D3D12FrameResource::~D3D12FrameResource()
    {

    }
    void D3D12FrameResource::PrepareForNewFrame()
    {
        D3D12::ThrowIfFailed(CommandAllocator->Reset());
        D3D12::ThrowIfFailed(CommandAllocator2->Reset());
    }
}