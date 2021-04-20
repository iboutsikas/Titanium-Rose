#include "hzpch.h"

#include "Hazel/Core/SystemTime.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/Profiler/GpuTime.h"
#include "Platform/D3D12/Profiler/Profiler.h"

#include "ImGui/imgui.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include "WinPixEventRuntime/pix3.h"
namespace Hazel {
    class StatHistory {
    public:
        StatHistory()
            : m_Average(0.0f), m_Minimum(0.0f), m_Maximum(0.0f), m_Recent(0.0f)
        {
            memset(m_RecentHistory, 0.0f, HistorySize * sizeof(float));
            memset(m_ExtendedHistory, 0.0f, ExtendedHistorySize * sizeof(float));
        }

        void RecordStat(uint32_t frame, float value)
        {
            m_RecentHistory[frame % HistorySize] = value;
            m_ExtendedHistory[frame % ExtendedHistorySize] = value;

            uint32_t count = 0;
            m_Minimum = std::numeric_limits<float>::max();
            m_Maximum = 0.0f;
            m_Average = 0.0f;

            for (float val : m_RecentHistory) {
                if (val > 0.0f) {
                    count++;
                    m_Average += val;
                    m_Minimum = std::min(val, m_Minimum);
                    m_Maximum = std::min(val, m_Maximum);
                }
            }

            if (count)
                m_Average /= (float)count;
            else
                m_Minimum = 0.0f;
        }

        float GetLast(void) const { return m_Recent; }
        float GetMax(void) const { return m_Maximum; }
        float GetMin(void) const { return m_Minimum; }
        float GetAvg(void) const { return m_Average; }

        const float* GetHistory(void) const { return m_ExtendedHistory; }
        uint32_t GetHistoryLength(void) const { return ExtendedHistorySize; }

    private:
        static constexpr uint32_t HistorySize = 64;
        static constexpr uint32_t ExtendedHistorySize = 256;
        float m_RecentHistory[HistorySize];
        float m_ExtendedHistory[ExtendedHistorySize];
        float m_Recent;
        float m_Average;
        float m_Minimum;
        float m_Maximum;
    };
    
    class GpuTimer {
    public:
        GpuTimer::GpuTimer() {
            m_TimerIndex = GpuTime::NewTimer();
        }

        void Start(Ref<D3D12CommandList> commandList) {
            GpuTime::StartTimer(commandList, m_TimerIndex);
        }

        void Stop(Ref<D3D12CommandList> commandList) {
            GpuTime::StopTimer(commandList, m_TimerIndex);
        }

        float GetTime(void) {
            return GpuTime::GetTime(m_TimerIndex);
        }

        uint32_t GetTimerIndex(void) {
            return m_TimerIndex;
        }

    private:
        uint32_t m_TimerIndex;
    };

    class NestedTimingTree {
    public:
        NestedTimingTree(const std::string& name, NestedTimingTree* parent = nullptr)
            : m_Name(name), m_Parent(parent)
        {
        }

        NestedTimingTree* GetChild(const std::string& name) {
            auto iter = m_Lookup.find(name);

            if (iter != m_Lookup.end())
                return iter->second;

            NestedTimingTree* node = new NestedTimingTree(name, this);
            m_Children.push_back(node);
            m_Lookup[name] = node;
            return node;
        }

        void StartTiming(Ref<D3D12CommandList> commandList) {
            m_StartTick = SystemTime::GetCurrentTick();
            if (commandList == nullptr)
                return;

            m_GpuTimer.Start(commandList);
            commandList->BeginEvent(m_Name.c_str());
        }

        void StopTiming(Ref<D3D12CommandList> commandList) {
            m_EndTick = SystemTime::GetCurrentTick();
            if (commandList == nullptr)
                return;

            m_GpuTimer.Stop(commandList);
            commandList->EndEvent();
        }

        void GatherTimes(uint32_t frame) {
            m_CPUTime.RecordStat(frame, 1000.0f * SystemTime::TimeBetweenTicks(m_StartTick, m_EndTick));
            m_GPUTime.RecordStat(frame, 1000.0f * m_GpuTimer.GetTime());

            for (auto child : m_Children)
                child->GatherTimes(frame);

            m_StartTick = 0;
            m_EndTick = 0;
        }

        void SumInclusiveTimes(float& cpuTime, float& gpuTime)
        {
            cpuTime = 0.0f;
            gpuTime = 0.0f;
            for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
            {
                cpuTime += (*iter)->m_CPUTime.GetLast();
                gpuTime += (*iter)->m_GPUTime.GetLast();
            }
        }
        

        static void NestedTimingTree::PushProfilingMarker(const std::string& name, Ref<D3D12CommandList> commandList);
        static void PopProfilingMarker(Ref<D3D12CommandList> commandList);

        static void UpdateTimes() {
            uint64_t frame = D3D12Renderer::GetFrameCount();
            GpuTime::BeginReadBack();
            s_RootScope.GatherTimes(frame);
            s_FrameDelta.RecordStat(frame, GpuTime::GetTime(0));
            GpuTime::EndReadBack();

            float totalCpuTime, totalGpuTime;
            s_RootScope.SumInclusiveTimes(totalCpuTime, totalGpuTime);
            s_TotalCpuTime.RecordStat(frame, totalCpuTime);
            s_TotalGpuTime.RecordStat(frame, totalGpuTime);

        }
    private:
        std::string m_Name;
        NestedTimingTree* m_Parent;
        std::vector<NestedTimingTree*> m_Children;
        std::unordered_map<std::string, NestedTimingTree*> m_Lookup;

        int64_t m_StartTick;
        int64_t m_EndTick;

        GpuTimer m_GpuTimer;
        
        StatHistory m_CPUTime;
        StatHistory m_GPUTime;


        static StatHistory s_TotalCpuTime;
        static StatHistory s_TotalGpuTime;
        static StatHistory s_FrameDelta;
        static NestedTimingTree s_RootScope;
        static NestedTimingTree* s_CurrentNode;
    };

    void Profiler::Initialize()
    {
        GpuTime::Initialize(4096);
    }

    void Profiler::Shutdown()
    {
        GpuTime::Shutdown();
    }

    void Profiler::Update()
    {
        NestedTimingTree::UpdateTimes();
    }

    void Profiler::Render()
    {

    }

    void Profiler::BeginBlock(const std::string& name, Ref<D3D12CommandList> commandList /*= nullptr*/)
    {
        NestedTimingTree::PushProfilingMarker(name, commandList);
    }

    void Profiler::EndBlock(Ref<D3D12CommandList> commandList /*= nullptr*/)
    {
        NestedTimingTree::PopProfilingMarker(commandList);
    }

}
namespace Hazel {
    NestedTimingTree NestedTimingTree::s_RootScope("");
    NestedTimingTree* NestedTimingTree::s_CurrentNode = &NestedTimingTree::s_RootScope;

    StatHistory NestedTimingTree::s_FrameDelta;
    StatHistory NestedTimingTree::s_TotalCpuTime;
    StatHistory NestedTimingTree::s_TotalGpuTime;


    void NestedTimingTree::PushProfilingMarker(const std::string& name, Ref<D3D12CommandList> commandList) {
        s_CurrentNode = s_CurrentNode->GetChild(name);
        s_CurrentNode->StartTiming(commandList);
    }

    void NestedTimingTree::PopProfilingMarker(Ref<D3D12CommandList> commandList) {
        s_CurrentNode->StopTiming(commandList);
        s_CurrentNode = s_CurrentNode->m_Parent;

    }
}
