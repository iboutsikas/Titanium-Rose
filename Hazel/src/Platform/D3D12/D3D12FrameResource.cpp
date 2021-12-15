#include "hzpch.h"
#include "Platform/D3D12/D3D12FrameResource.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Hazel {
    D3D12FrameResource::D3D12FrameResource(TComPtr<ID3D12Device> device, UINT passCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CommandAllocator.GetAddressOf())));
    }

    D3D12FrameResource::~D3D12FrameResource()
    {

    }
}