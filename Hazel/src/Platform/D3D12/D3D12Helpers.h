#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#include <system_error>
#include <string>
#include <d3d12.h>

#include "Hazel/Core/Log.h"
namespace Hazel::D3D12 {

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
        INTEL = 0x8086
    };
}