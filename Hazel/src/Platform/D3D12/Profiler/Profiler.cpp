#include "hzpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/Profiler/Profiler.h"

#include "ImGui/imgui.h"

namespace Hazel
{


    Hazel::Profiler Profiler::GlobalProfiler;

    struct ProfileData
    {
        static constexpr uint64_t FilterSize = 64;
        std::string Name;

        bool QueryStarted = false;
        bool QueryFinished = false;
        bool Active = false;

        bool CPUProfile = false;
        uint64_t StartTime = 0;
        uint64_t EndTime = 0;

        double TimeSamples[FilterSize] = { };
        uint64_t CurrSample = 0;

        double AverageTime = 0;
        double MaxTime = 0;
    };


    void Profiler::Initialize()
    {
        Shutdown();
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Count = MaxProfiles * 2;
        desc.NodeMask = 0;
        desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        D3D12Renderer::Context->DeviceResources
            ->Device->CreateQueryHeap(&desc, IID_PPV_ARGS(queryHeap.GetAddressOf()));
        // MaxProfiles for every Frame, and 2 timestamps one for start and one for end
        readbackBuffer = CreateScope<ReadbackBuffer>(MaxProfiles * D3D12Renderer::FrameLatency * 2 * sizeof(uint64_t));

        m_GPUProfiles.resize(MaxProfiles);
        m_CPUProfiles.resize(MaxProfiles);
    }

    void Profiler::Shutdown()
    {
        queryHeap.Reset();
        readbackBuffer.reset();
        m_GPUProfiles.clear();
        m_CPUProfiles.clear();
        m_GPUCount = m_CPUCount = 0;
    }

    uint64_t Profiler::StartProfile(ID3D12GraphicsCommandList* cmdlist, const std::string& name)
    {
        HZ_CORE_ASSERT(!name.empty(), "Profile name is empty");

        // uint_max
        uint64_t idx = uint64_t(-1);
        for (uint64_t i = 0; i < m_GPUCount; ++i)
        {
            if (m_GPUProfiles[i].Name == name)
            {
                idx = i;
                break;
            }
        }

        // We do not have an existing profile. Let's try to add it
        if (idx == uint64_t(-1))
        {
            HZ_CORE_ASSERT(m_GPUCount < MaxProfiles, "We are out of profiles!");
            idx = m_GPUCount++;
            m_GPUProfiles[idx].Name = name;
        }

        ProfileData& datum = m_GPUProfiles[idx];
        HZ_CORE_ASSERT(datum.QueryStarted == false, "");
        HZ_CORE_ASSERT(datum.QueryFinished == false, "");
        datum.CPUProfile = false;
        datum.Active = true;

        const uint32_t startQueryIdx = uint32_t(idx * 2);
        cmdlist->EndQuery(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, startQueryIdx);

        datum.QueryStarted = true;
        return idx;
    }

    void Profiler::EndProfile(ID3D12GraphicsCommandList* cmdlist, uint64_t idx)
    {
        HZ_CORE_ASSERT(idx < m_GPUCount, "Index is out of bounds");

        auto& datum = m_GPUProfiles[idx];
        const uint32_t startIdx = uint32_t(idx * 2);
        const uint32_t endIdx = startIdx + 1;
        cmdlist->EndQuery(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, endIdx);

        auto frameIndex = D3D12Renderer::Context->GetCurrentFrameIndex();
        
        const uint64_t dest = ((frameIndex * MaxProfiles * 2) + startIdx) * sizeof(uint64_t);
        cmdlist->ResolveQueryData(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, startIdx, 2, readbackBuffer->GetResource(), dest);

        datum.QueryStarted = false;
        datum.QueryFinished = true;
    }

    uint64_t Profiler::StartCPUProfile(const std::string& name)
    {
        HZ_CORE_ASSERT(!name.empty(), "Profile name is empty");
        uint64_t idx = uint64_t(-1);
        for (uint64_t i = 0; i < m_CPUCount; ++i)
        {
            if (m_CPUProfiles[i].Name == name)
            {
                idx = i;
                break;
            }
        }

        // We do not have an existing profile. Let's try to add it
        if (idx == uint64_t(-1))
        {
            HZ_CORE_ASSERT(m_CPUCount < MaxProfiles, "We are out of profiles!");
            idx = m_CPUCount++;
            m_CPUProfiles[idx].Name = name;
        }

        auto& datum = m_CPUProfiles[idx];
        HZ_CORE_ASSERT(datum.QueryStarted == false, "");
        HZ_CORE_ASSERT(datum.QueryFinished == false, "");
        datum.CPUProfile = true;
        datum.Active = true;
        timer.Tick();

        datum.StartTime = timer.ElapsedMicroseconds();
        datum.QueryStarted = true;

        return idx;
    }

    void Profiler::EndCPUProfile(uint64_t idx)
    {
        HZ_CORE_ASSERT(idx < m_CPUCount, "Index is out of bounds");

        auto& datum = m_CPUProfiles[idx];

        HZ_CORE_ASSERT(datum.QueryStarted == true, "");
        HZ_CORE_ASSERT(datum.QueryFinished == false, "");

        timer.Tick();

        datum.EndTime = timer.ElapsedMicroseconds();
        datum.QueryStarted = false;
        datum.QueryFinished = true;
    }

    void Profiler::EndFrame(uint32_t displayWidth, uint32_t displayHeight)
    {
        uint64_t gpuFrequency = 0;
        uint64_t* data = nullptr;

        D3D12Renderer::Context->DeviceResources->CommandQueue->GetTimestampFrequency(&gpuFrequency);
        uint64_t* queryData = readbackBuffer->Map<uint64_t*>();

        data = queryData + (D3D12Renderer::Context->GetCurrentFrameIndex() * MaxProfiles * 2);

        // Update all the GPU profiles
        for (uint64_t p = 0; p < m_GPUCount; ++p)
        {
            UpdateProfile(m_GPUProfiles[p], p, gpuFrequency, data);
        }
        // Update all the CPU profiles
        for (uint64_t p = 0; p < m_CPUCount; ++p)
        {
            UpdateProfile(m_CPUProfiles[p], p, gpuFrequency, nullptr);
        }

        readbackBuffer->Unmap();
    }

    void Profiler::RenderStats()
    {
        ImGui::Text("CPU Timing");
        for (uint64_t p = 0; p < m_CPUCount; ++p)
        {
            auto& profile = m_CPUProfiles[p];
            ImGui::Text("%s: %.2f ms (%.2f ms max)", profile.Name.c_str(), profile.AverageTime, profile.MaxTime);
        }

        ImGui::Separator();
        ImGui::Text("GPU Timings");
        for (uint64_t p = 0; p < m_GPUCount; ++p)
        {
            auto& profile = m_GPUProfiles[p];
            ImGui::Text("%s: %.3f ms (%.3f ms max)", profile.Name.c_str(), profile.AverageTime, profile.MaxTime);
        }

    }

    void Profiler::UpdateProfile(ProfileData& profile, uint64_t index, uint64_t gpuFrequency, const uint64_t* frameQueryData)
    {
        profile.QueryFinished = false;

        double time = 0.0;
        if (profile.CPUProfile)
        {
            time = double(profile.EndTime - profile.StartTime) * 0.001;
        }
        else if (frameQueryData != nullptr)
        {
            auto startTimeStamp = frameQueryData[index * 2];
            auto endTimeStamp   = frameQueryData[index * 2 + 1];

            if (endTimeStamp > startTimeStamp)
            {
                uint64_t delta = endTimeStamp - startTimeStamp;
                double freq = double(gpuFrequency);
                time = (delta / freq) * 1000.0;
            }
        }

        profile.TimeSamples[profile.CurrSample] = time;
        profile.CurrSample = (profile.CurrSample + 1) % ProfileData::FilterSize;
        
        double maxTime = 0.0;
        double avgTime = 0.0;
        uint64_t samplesTaken= 0;

        for (uint32_t s = 0; s < ProfileData::FilterSize; ++s)
        {
            if (profile.TimeSamples[s] <= 0.0)
            {
                // No data yet, ignore it
                continue;
            }
            
            maxTime = std::max<double>(profile.TimeSamples[s], maxTime);
            avgTime += profile.TimeSamples[s];
            ++samplesTaken;
        }

        if (samplesTaken > 0)
            avgTime /= (double)samplesTaken;

        profile.AverageTime = avgTime;
        profile.MaxTime = maxTime;

        profile.Active = false;
    }

    //====================================Profile Block=============================================

    GPUProfileBlock::GPUProfileBlock(ID3D12GraphicsCommandList* cmdlist, const std::string& name)
        : m_Cmdlist(cmdlist)
    {
        m_Index = Profiler::GlobalProfiler.StartProfile(cmdlist, name);
    }

    GPUProfileBlock::GPUProfileBlock(const GPUProfileBlock& other)
        : m_Index(other.m_Index), m_Cmdlist(other.m_Cmdlist), m_ShouldEnd(false)
    {     
        //HZ_CORE_WARN("GPU profile copy constructor. Should not be called");
    }

    GPUProfileBlock::GPUProfileBlock(GPUProfileBlock& other)
        : m_Index(other.m_Index), m_Cmdlist(other.m_Cmdlist), m_ShouldEnd(true)
    {
        //HZ_CORE_WARN("GPU profile move constructor");
        other.m_ShouldEnd = false;
    }

    GPUProfileBlock::~GPUProfileBlock()
    {
        if (m_ShouldEnd)
            Profiler::GlobalProfiler.EndProfile(m_Cmdlist, m_Index);
    }

    CPUProfileBlock::CPUProfileBlock(const std::string& name)
    {
        m_Index = Profiler::GlobalProfiler.StartCPUProfile(name);
    }

    CPUProfileBlock::~CPUProfileBlock()
    {
        Profiler::GlobalProfiler.EndCPUProfile(m_Index);
    }

}