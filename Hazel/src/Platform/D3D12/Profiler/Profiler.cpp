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
            m_Recent = value;

            uint32_t count = 0;
            m_Minimum = std::numeric_limits<float>::max();
            m_Maximum = 0.0f;
            m_Average = 0.0f;

            for (float val : m_RecentHistory) {
                if (val > 0.0f) {
                    count++;
                    m_Average += val;
                    m_Minimum = std::min(val, m_Minimum);
                    m_Maximum = std::max(val, m_Maximum);
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

        void Start(CommandContext& context) {
            GpuTime::StartTimer(context, m_TimerIndex);
        }

        void Stop(CommandContext& context) {
            GpuTime::StopTimer(context, m_TimerIndex);
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
        friend class Profiler;
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

        void StartTiming(CommandContext* context) {
            m_StartTick = SystemTime::GetCurrentTick();
            if (context == nullptr) {
                // We care about the CPU markers as well. So we start the pix even directly if no contex is given
                PIXBeginEvent(PIX_COLOR_INDEX(m_GpuTimer.GetTimerIndex()), m_Name.c_str());
                return;
            }

            m_GpuTimer.Start(*context);
            context->BeginEvent(m_Name.c_str(), PIX_COLOR_INDEX(m_GpuTimer.GetTimerIndex()));
        }

        void StopTiming(CommandContext* context) {

            m_EndTick = SystemTime::GetCurrentTick();
            //HZ_CORE_TRACE("StopTiming: {0}", m_EndTick);
            if (context == nullptr) {
                PIXEndEvent();
                return;
            }

            m_GpuTimer.Stop(*context);
            context->EndEvent();
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
        
        void RenderSelf(GraphicsContext& gfxContext);

        static void NestedTimingTree::PushProfilingMarker(const std::string& name, CommandContext* context);
        static void PopProfilingMarker(CommandContext* context);

        static void Render(GraphicsContext& gfxContext);

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

        static float GetCPUAverage() { return s_TotalCpuTime.GetAvg(); }
        static float GetGPUAverage() { return s_TotalGpuTime.GetAvg(); }
        static float GetFrameDelta() { return s_FrameDelta.GetAvg(); }
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

    void Profiler::Render(CommandContext& context)
    {
        GraphicsContext& gfxContext = context.GetGraphicsContext();

        float cpuTime = NestedTimingTree::GetCPUAverage();
        float gpuTime = NestedTimingTree::GetGPUAverage();
        float frameDelta = NestedTimingTree::GetFrameDelta();
        float fps = 1000.0f / cpuTime;

        ImGui::Text("CPU %7.4f ms, GPU %7.4f ms, %4.2f FPS, Delta %7.4f\n",
            cpuTime, gpuTime, fps, frameDelta);

        NestedTimingTree::Render(gfxContext);
    }

    void Profiler::BeginBlock(const std::string& name, CommandContext* context /*= nullptr*/)
    {
        NestedTimingTree::PushProfilingMarker(name, context);
    }

    void Profiler::EndBlock(CommandContext* context /*= nullptr*/)
    {
        NestedTimingTree::PopProfilingMarker(context);
    }

    void Profiler::SaveTimings(const std::string& blockName, const std::string& filePath) {
        using json = nlohmann::json;

        auto node = NestedTimingTree::s_RootScope.GetChild(blockName);

        HZ_CORE_ASSERT(node != nullptr, "This shouldn't really ever happen");

        std::fstream file(filePath, std::fstream::out);
        if (!file.is_open()) {
            HZ_CORE_ERROR("Unable to open file: {0}", filePath);
            return;
        }

        json output;

        auto obj = json::object();
        obj["block_name"] = blockName;
        auto cpuObj = json::object();
        cpuObj["min"] = node->m_CPUTime.GetMin();
        cpuObj["max"] = node->m_CPUTime.GetMax();
        cpuObj["avg"] = node->m_CPUTime.GetAvg();
        cpuObj["values"] = json::array();

        for (int i = 0; i < node->m_CPUTime.GetHistoryLength(); i++)
        {
            cpuObj["values"].emplace_back(node->m_CPUTime.GetHistory()[i]);
        }

        obj["cpu"] = cpuObj;

        auto gpuObj = json::object();
        gpuObj["min"] = node->m_GPUTime.GetMin();
        gpuObj["max"] = node->m_GPUTime.GetMax();
        gpuObj["avg"] = node->m_GPUTime.GetAvg();
        gpuObj["values"] = json::array();

        for (int i = 0; i < node->m_GPUTime.GetHistoryLength(); i++)
        {
            gpuObj["values"].emplace_back(node->m_GPUTime.GetHistory()[i]);
        }
        
        obj["gpu"] = gpuObj;

        file << obj.dump(4);
        file.close();
    }

}
namespace Hazel {
    NestedTimingTree NestedTimingTree::s_RootScope("");
    NestedTimingTree* NestedTimingTree::s_CurrentNode = &NestedTimingTree::s_RootScope;

    StatHistory NestedTimingTree::s_FrameDelta;
    StatHistory NestedTimingTree::s_TotalCpuTime;
    StatHistory NestedTimingTree::s_TotalGpuTime;


    void NestedTimingTree::PushProfilingMarker(const std::string& name, CommandContext* context) {
        auto temp = s_CurrentNode->GetChild(name);
        HZ_CORE_ASSERT(temp != nullptr, "");
        s_CurrentNode = temp;
        s_CurrentNode->StartTiming(context);
    }

    void NestedTimingTree::PopProfilingMarker(CommandContext* context) {
        s_CurrentNode->StopTiming(context);
        s_CurrentNode = s_CurrentNode->m_Parent;
    }

    void NestedTimingTree::Render(GraphicsContext& gfxContext) {
        s_RootScope.RenderSelf(gfxContext);
    }

    void NestedTimingTree::RenderSelf(GraphicsContext& gfxContext) {
        if (!m_Name.length()) {
            for (auto child : m_Children)
            {
                child->RenderSelf(gfxContext);
            }
            return;
        }

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (m_Children.size() == 0) {
            nodeFlags |= ImGuiTreeNodeFlags_Leaf; // | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (ImGui::TreeNodeEx(m_Name.c_str(), nodeFlags, "%s CPU: %7.4f ms GPU %7.4f ms", m_Name.c_str(), m_CPUTime.GetAvg(), m_GPUTime.GetAvg())) {

            for (auto child : m_Children)
            {
                child->RenderSelf(gfxContext);
            }

            ImGui::TreePop();
        }
    }
}
