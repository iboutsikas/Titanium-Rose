#pragma once
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/vec4.hpp>

namespace Hazel {
    class D3D12Context;

    class D3D12RendererAPI : public RendererAPI
    {
    public:

        virtual void Init() override;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        virtual void SetClearColor(const glm::vec4& color) override;
        virtual void Clear() override;

        virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) override;

        virtual void BeginFrame() override;
        virtual void EndFrame() override;

    private:
        glm::vec4 m_ClearColor;
        D3D12Context* Context;

    };
}