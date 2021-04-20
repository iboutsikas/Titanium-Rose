#pragma once
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/GpuResource.h"
namespace Hazel {
    class GpuBuffer: public GpuResource
    {
    public:
        GpuBuffer();
        GpuBuffer(uint32_t width, uint32_t height, uint32_t depth, std::string id, D3D12_RESOURCE_STATES state);
        
        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint32_t GetDepth() const { return m_Depth; }

    protected:
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Depth;   
    };
}


