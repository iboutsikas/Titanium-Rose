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

namespace Hazel {
    class Profiler {
    public:
        static void Initialize();
        static void Shutdown();

        static void Update();
        static void Render();

        static void BeginBlock(const std::string& name, Ref<D3D12CommandList> commandList = nullptr);
        static void EndBlock(Ref<D3D12CommandList> commandList = nullptr);
    };

    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name) : m_CommandList(nullptr) {
            Profiler::BeginBlock(name);
        }

        ScopedTimer(const std::string& name, Ref<D3D12CommandList> commandList) : m_CommandList(commandList) {
            Profiler::BeginBlock(name, commandList);
        }

        ~ScopedTimer() {
            Profiler::EndBlock(m_CommandList);
        }

    private:
        Ref<D3D12CommandList> m_CommandList;
    };
}