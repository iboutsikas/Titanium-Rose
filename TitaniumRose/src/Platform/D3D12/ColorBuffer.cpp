#include "trpch.h"
#include "Platform/D3D12/ColorBuffer.h"
#include "Platform/D3D12/D3D12Renderer.h"

namespace Roses {
    
    ColorBuffer::ColorBuffer(glm::vec4 clearColor /*= { 0, 0, 0, 0 }*/)
        : m_ClearColor(clearColor), m_Format(DXGI_FORMAT_UNKNOWN)
    {

    }

    void ColorBuffer::CreateFromSwapChain(const std::string& name, ID3D12Resource* swapChainBuffer)
    {
        auto desc = swapChainBuffer->GetDesc();

        m_Resource.Attach(swapChainBuffer);
        m_CurrentState = D3D12_RESOURCE_STATE_PRESENT;

        m_Width = desc.Width;
        m_Height = desc.Height;
        m_Depth = desc.DepthOrArraySize;
        m_Format = desc.Format;

        m_RTVAllocation = D3D12Renderer::s_RenderTargetDescriptorHeap->Allocate(1);
        HZ_CORE_ASSERT(m_RTVAllocation.Allocated, "RTV allocation failed");
        D3D12Renderer::GetDevice()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_RTVAllocation.CPUHandle);
        SetName(name);
    }

}
