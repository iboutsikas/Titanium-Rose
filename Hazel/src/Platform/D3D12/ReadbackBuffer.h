#pragma once
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/DeviceResource.h"

namespace Hazel
{
    class ReadbackBuffer : public DeviceResource
    {
    public:
        ReadbackBuffer(uint64_t size);
        ReadbackBuffer(Ref<DeviceResource> resource);

        inline uint32_t GetSize() const { return m_Width * m_Height * m_Depth; }

        void* Map();
        void Unmap();
        
        template<typename T,  typename = std::is_pointer<T>()>
        const T Map() { return reinterpret_cast<T>(Map()); }


    private:
        void* m_Data;
    };
}
