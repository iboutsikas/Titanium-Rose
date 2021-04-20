#include "hzpch.h"
#include "SystemTime.h"

namespace Hazel {
    double SystemTime::s_CpuTickDelta = 0.0;


    void SystemTime::Initialize(void)
    {
        LARGE_INTEGER frequency;
        HZ_CORE_ASSERT(TRUE == QueryPerformanceFrequency(&frequency), "Unable to query performance counter frequency");
        s_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
    }

    int64_t SystemTime::GetCurrentTick(void)
    {
        LARGE_INTEGER currentTick;
        HZ_CORE_ASSERT(TRUE == QueryPerformanceCounter(&currentTick), "Unable to query performance counter value");
        return static_cast<int64_t>(currentTick.QuadPart);
    }

}