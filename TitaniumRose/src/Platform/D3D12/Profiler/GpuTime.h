
#include "Platform/D3D12/CommandContext.h"

namespace Roses{ namespace GpuTime {
    void Initialize(uint32_t maxTimers = 4096);
    void Shutdown();

    uint32_t NewTimer(void);

    void StartTimer(CommandContext& context, uint32_t index);
    void StopTimer(CommandContext& context, uint32_t index);

    void BeginReadBack(void);
    void EndReadBack(void);
    
    // In milliseconds
    float GetTime(uint32_t TimerIdx);
}}