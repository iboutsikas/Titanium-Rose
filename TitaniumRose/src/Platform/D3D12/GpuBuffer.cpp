#include "trpch.h"
#include "Platform/D3D12/GpuBuffer.h"

namespace Roses {
    GpuBuffer::GpuBuffer():
        GpuResource(),
        m_Width(0), m_Height(0), m_Depth(0)
    {
    }
    GpuBuffer::GpuBuffer(uint32_t width, uint32_t height, uint32_t depth, std::string id, D3D12_RESOURCE_STATES state):
        GpuResource(id, state),
        m_Width(width), m_Height(height), m_Depth(depth)
    {
    }
}