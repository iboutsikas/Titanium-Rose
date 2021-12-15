#pragma once
#include "TitaniumRose/Core/Layer.h"

class EmptyLayer: public Roses::Layer
{
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Roses::Timestep ts) override;
    virtual void OnRender(Roses::Timestep ts, Roses::GraphicsContext& gfxContext) override;

};

