#include "hzpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/ReadbackBuffer.h"

namespace Hazel
{
    ReadbackBuffer::ReadbackBuffer(uint64_t size)
        : DeviceResource(size, 1, 1, "", D3D12_RESOURCE_STATE_COPY_DEST), m_Data(nullptr)
    {
        HZ_CORE_ASSERT(size > 0, "Cannot have a readback buffer with 0 size");

        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_Width);



        D3D12::ThrowIfFailed(D3D12Renderer::Context->DeviceResources->Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(m_Resource.GetAddressOf())
        ));
        
    }

    ReadbackBuffer::ReadbackBuffer(Ref<DeviceResource> deviceResource)
        : DeviceResource(deviceResource->GetWidth(), deviceResource->GetHeight(), deviceResource->GetDepth(),
            "", D3D12_RESOURCE_STATE_COPY_DEST), m_Data(nullptr)
    {
        auto pair_desc = deviceResource->GetResource()->GetDesc();

        D3D12_RESOURCE_DESC desc = pair_desc;
        desc.MipLevels = 1;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12::ThrowIfFailed(D3D12Renderer::Context->DeviceResources->Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(m_Resource.GetAddressOf())
        ));
    }

    void* ReadbackBuffer::Map()
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Trying to map null resource");
        m_Resource->Map(0, nullptr, &m_Data);
        return m_Data;
    }

    void ReadbackBuffer::Unmap()
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Trying to unmap null resource");
        m_Resource->Unmap(0, nullptr);
    }

}
