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
        static void Render(CommandContext& context);

        static void BeginBlock(const std::string& name, CommandContext* context = nullptr);
        static void EndBlock(CommandContext* context = nullptr);
    };

    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name) : m_Context(nullptr) {
            Profiler::BeginBlock(name);
        }

        ScopedTimer(const std::string& name, CommandContext& context) : m_Context(&context) {
            Profiler::BeginBlock(name, m_Context);
        }

        ~ScopedTimer() {
            Profiler::EndBlock(m_Context);
        }

    private:
        CommandContext* m_Context;
    };
}