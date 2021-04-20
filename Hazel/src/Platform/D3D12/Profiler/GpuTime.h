
#include "Platform/D3D12/D3D12CommandList.h"


namespace Hazel{ namespace GpuTime {
    void Initialize(uint32_t maxTimers = 4096);
    void Shutdown();

    uint32_t NewTimer(void);

    void StartTimer(Ref<D3D12CommandList> commandList, uint32_t index);
    void StopTimer(Ref<D3D12CommandList> commandList, uint32_t index);

    void BeginReadBack(void);
    void EndReadBack(void);
    
    // In milliseconds
    float GetTime(uint32_t TimerIdx);
}}