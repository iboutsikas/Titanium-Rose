#pragma once
namespace Roses {
    class SystemTime
    {
    public:
        static void Initialize(void);
        static int64_t GetCurrentTick(void);

        static inline double TicksToSeconds(int64_t TickCount)
        {
            return TickCount * s_CpuTickDelta;
        }

        static inline double TicksToMillisecs(int64_t TickCount)
        {
            return TickCount * s_CpuTickDelta * 1000.0;
        }

        static inline double TimeBetweenTicks(int64_t tick1, int64_t tick2)
        {
            return TicksToSeconds(tick2 - tick1);
        }

    private:
        static double s_CpuTickDelta;
    };

}


