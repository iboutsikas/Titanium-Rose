#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Timer.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/ReadbackBuffer.h"

#include <d3d12.h>
#include <vector>

namespace Hazel
{
    struct ProfileData;

    class GPUProfileBlock
    {
    public:
        GPUProfileBlock(ID3D12GraphicsCommandList* cmdlist, const std::string& name);
        GPUProfileBlock(const GPUProfileBlock& other);
        GPUProfileBlock(GPUProfileBlock& other);

        ~GPUProfileBlock();
    private:
        ID3D12GraphicsCommandList* m_Cmdlist;
        uint64_t m_Index = uint64_t(-1);
        bool m_ShouldEnd = true;
    };

    class CPUProfileBlock
    {
    public:
        CPUProfileBlock(const std::string& name);
        ~CPUProfileBlock();
    private:
        uint64_t m_Index = uint64_t(-1);
    };

    class Profiler
    {
    public:
        static constexpr uint32_t MaxProfiles = 64;
        static Profiler GlobalProfiler;

        void Initialize();
        void Shutdown();

        uint64_t StartProfile(ID3D12GraphicsCommandList* cmdList, const std::string& name);
        void EndProfile(ID3D12GraphicsCommandList* cmdList, uint64_t idx);

        uint64_t StartCPUProfile(const std::string& name);
        void EndCPUProfile(uint64_t idx);

        void EndFrame(uint32_t displayWidth, uint32_t displayHeight);

        void RenderStats();
    private:
        static void UpdateProfile(ProfileData& profile, uint64_t index, uint64_t gpuFrequency, const uint64_t* frameQueryData);
    private:

        std::vector<ProfileData> m_GPUProfiles;
        std::vector<ProfileData> m_CPUProfiles;
        uint64_t m_GPUCount = 0;
        uint64_t m_CPUCount = 0;
        Timer timer;
        TComPtr<ID3D12QueryHeap> queryHeap = nullptr;
        Scope<ReadbackBuffer> readbackBuffer;
    };

  

}