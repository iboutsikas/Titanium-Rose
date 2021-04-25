#pragma once

#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel
{
    struct FrameBufferOptions {
        uint32_t Width;
        uint32_t Height;
        DXGI_FORMAT ColourFormat;
        DXGI_FORMAT DepthStencilFormat;
        std::string Name;

        FrameBufferOptions(uint32_t width, uint32_t height, DXGI_FORMAT colourFormat, DXGI_FORMAT depthStencilFormat, std::string& name) :
            Width(width), Height(height), ColourFormat(colourFormat), DepthStencilFormat(depthStencilFormat), Name(std::move(name))
        {}

        FrameBufferOptions() :
            Width(1), Height(1), ColourFormat(DXGI_FORMAT_UNKNOWN), DepthStencilFormat(DXGI_FORMAT_UNKNOWN), Name("")
        {}
    };

    class FrameBuffer
    {
    public:

        Ref<Texture2D> ColorResource;
        Ref<Texture2D> DepthStensilResource;
        HeapAllocationDescription RTVAllocation;
        HeapAllocationDescription SRVAllocation;
        HeapAllocationDescription DSVAllocation;

        static Ref<FrameBuffer> Create(FrameBufferOptions& opts);

    private:
        FrameBuffer();

        
    };

}

