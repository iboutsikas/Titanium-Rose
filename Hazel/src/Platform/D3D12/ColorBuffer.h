#pragma once
#include "Platform/D3D12/GpuBuffer.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"

#include <glm/glm.hpp>

namespace Hazel {
    class ColorBuffer: public GpuBuffer
    {
    public:
        ColorBuffer(glm::vec4 clearColor = { 0, 0, 0, 0 });

        void CreateFromSwapChain(const std::string& name, ID3D12Resource* swapChainBuffer);

        void SetClearColor(glm::vec4 newColor) { m_ClearColor = newColor; }
        glm::vec4 GetClearColor() const { return m_ClearColor; }
        const DXGI_FORMAT& GetFormat() const { return m_Format; }

    private:
        glm::vec4 m_ClearColor;
        DXGI_FORMAT m_Format;
        HeapAllocationDescription m_RTVAllocation;
    };
}


