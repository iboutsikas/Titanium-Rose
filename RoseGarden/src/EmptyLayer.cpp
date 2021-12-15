#include "EmptyLayer.h"

#include "Platform/D3D12/D3D12Renderer.h"

#include "glm/glm.hpp"

void EmptyLayer::OnAttach()
{
}

void EmptyLayer::OnDetach()
{
}
static glm::vec4 color(1, 0, 1, 1);
void EmptyLayer::OnUpdate(Roses::Timestep ts)
{
    
    //
}

void EmptyLayer::OnRender(Roses::Timestep ts, Roses::GraphicsContext& gfxContext)
{
    using namespace Roses;

    D3D12Renderer::PrepareBackBuffer(gfxContext, color);

    D3D12Renderer::DoToneMapping(gfxContext);
}
