#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Timer.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/ReadbackBuffer.h"
#include "Platform/D3D12/D3D12CommandList.h"

#include <vector>
#include <list>
#include <unordered_map>

#include "nlohmann/json.hpp"

struct ID3D12QueryHeap;

namespace Hazel
{
    struct ProfileData;

    class ProfileBlock 
    {
    public:
        ProfileBlock(bool shouldEnd, uint64_t index = -1);
        virtual ~ProfileBlock() = default;

    protected:
        uint64_t m_Index;
        bool m_ShouldEnd;
    };

    class GPUProfileBlock: public ProfileBlock
    {
    public:
        GPUProfileBlock();
        GPUProfileBlock(Ref<D3D12CommandList> commandList, const std::string& name);
        GPUProfileBlock(GPUProfileBlock&& other);
        virtual ~GPUProfileBlock();
    private:
        GPUProfileBlock(const GPUProfileBlock& other) = delete;

    private:
        Ref<D3D12CommandList> m_Cmdlist;
    };

    class CPUProfileBlock: public ProfileBlock
    {
    public:
        CPUProfileBlock();
        CPUProfileBlock(const std::string& name);
        CPUProfileBlock(CPUProfileBlock&& other);
        virtual ~CPUProfileBlock();
    private:
        CPUProfileBlock(const CPUProfileBlock& other) = delete;

    };

    class Profiler
    {
    public:
        static constexpr uint32_t MaxProfiles = 64;
        static Profiler GlobalProfiler;

        void Initialize();
        void Shutdown();

        uint64_t StartProfile(Ref<D3D12CommandList> commandList, const std::string& name);
        void EndProfile(Ref<D3D12CommandList> commandList, uint64_t idx);

        uint64_t StartCPUProfile(const std::string& name);
        void EndCPUProfile(uint64_t idx);

        void EndGPUFrame();
        void EndCPUFrame();

        void RenderStats();

        void DumpDatabases(std::string& path);

    private:
        void UpdateProfile(ProfileData& profile, uint64_t index, uint64_t gpuFrequency, const uint64_t* frameQueryData);

    private:
        struct SerializedProfileData {
            uint64_t StartTime = 0;
            uint64_t EndTime = 0;
            uint64_t Frame = 0;
            // The delta is in milliseconds
            double Delta = 0.0;
        };

    public:
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializedProfileData, StartTime, EndTime, Frame, Delta)
    private:

        std::vector<ProfileData> m_GPUProfiles;
        std::vector<ProfileData> m_CPUProfiles;
        uint64_t m_GPUCount = 0;
        uint64_t m_CPUCount = 0;
        uint64_t m_FrameCount = 0;
        Timer timer;
        TComPtr<ID3D12QueryHeap> m_QueryHeap = nullptr;
        Scope<ReadbackBuffer> m_ReadbackBuffer;

        std::unordered_map<std::string, std::list<SerializedProfileData>> m_GPUDatabase;
        std::unordered_map<std::string, std::list<SerializedProfileData>> m_CPUDatabase;
    };

}