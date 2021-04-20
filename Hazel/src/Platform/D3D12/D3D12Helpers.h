#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#include <system_error>
#include <string>
#include <d3d12.h>
#include <cmath>
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#include "Hazel/Core/Log.h"
namespace Hazel::D3D12 {
        
    inline float ConvertFromFP16toFP32(uint16_t fp16) 
    {
        // gpu fp16 is:
        // 1 bit sign
        // 5 bits exponent
        // 10 bits mantissa

        // We grab the mantissa first, as we might need it for adjustment
        uint32_t mantissa = static_cast<uint32_t>(fp16 & 0x03ff);

        uint32_t exponent = (fp16 & 0x7c00);

        
        if (exponent == 0x07c00) 
        {
            // Handle INF/NAN
            exponent = 0x8f;
        }
        else if (exponent != 0)
        {
            // the half is normalized
            exponent = (fp16 >> 10) & 0x1f;
        }
        else if (mantissa != 0)
        {
            // fp16 is denormalized
            exponent = 1;
            do 
            {
                exponent = exponent - 1;
                mantissa <<= 1;
            } while ((mantissa & 0x0400) == 0);

            mantissa &= 0x03ff;
        }
        else
        {
            // fp16 is 0
            exponent = static_cast<uint32_t>(-112);
        }

        exponent = (exponent + 112) << 23;

        mantissa <<= 13;

        uint32_t sign = (fp16 & 0x08000) << 16;

        auto fp32 = sign | exponent | mantissa;

        return reinterpret_cast<float*>(&fp32)[0];
    }

    inline uint32_t CalculateMips(uint32_t width, uint32_t height) {

        return 1 + (uint32_t)std::floor(std::log10((float)std::max<uint32_t>(width, height)) / std::log10(2.0));
    }

    inline int32_t RoundToMultiple(int32_t number, int32_t multiple) {
        return(number + multiple - 1) / multiple;
    }

    template<typename T>
    __forceinline T AlignUpMasked(T byteSize, size_t mask)
    {
        return (T)(((size_t)byteSize + mask) & ~mask);
    }

    template<typename T>
    __forceinline T AlignUp(T byteSize, size_t alignment = 256)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return AlignUpMasked(byteSize, alignment - 1);
    }

    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            std::string message = std::system_category().message(hr);
            HZ_CORE_ASSERT(false, message);
        }
    }

#if defined(HZ_DEBUG)
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
    {
        WCHAR fullName[50];
        if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
        {
            pObject->SetName(fullName);
        }
    }
#else
    inline void SetName(ID3D12Object*, LPCWSTR)
    {
    }
    inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
    {
    }
#endif

#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)
#define NAME_D3D12_OBJECT_WITH_INDEX(x, n) SetNameIndexed((x).Get(), L#x, n)

    enum class VendorID : UINT {
        AMD = 0x1002,
        NVIDIA = 0x10DE,
        INTEL = 0x8086,
        CAPTURE = 0x1414
    };
}