#pragma once
#include "TitaniumRose/Core/Core.h"

#include "Platform/D3D12/GpuBuffer.h"

namespace Roses
{
    class ReadbackBuffer : public GpuBuffer
    {
    public:
        ReadbackBuffer(uint64_t size);
        //ReadbackBuffer(Ref<DeviceResource> resource);

        inline uint32_t GetSize() const { return m_Width * m_Height * m_Depth; }

        void* Map(size_t begin = 0, size_t end = 0);
        void Unmap(size_t begin = 0, size_t end = 0);
        
        template<typename T,  typename = std::is_pointer<T>()>
        const T Map(size_t begin = 0, size_t end = 0) { return reinterpret_cast<T>(Map(begin, end)); }


    private:
        void* m_Data;
    };
}
