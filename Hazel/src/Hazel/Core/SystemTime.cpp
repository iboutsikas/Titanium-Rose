#include "hzpch.h"
#include "SystemTime.h"

#include "Platform/D3D12/D3D12Helpers.h"

namespace Hazel {
    double SystemTime::s_CpuTickDelta = 0.0;


    void SystemTime::Initialize(void)
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        HZ_CORE_INFO("Frequency.QuadPart: {0}", frequency.QuadPart);
        s_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
    }

    int64_t SystemTime::GetCurrentTick(void)
    {
        LARGE_INTEGER currentTick;
        QueryPerformanceCounter(&currentTick);

        return static_cast<int64_t>(currentTick.QuadPart);
    }

}