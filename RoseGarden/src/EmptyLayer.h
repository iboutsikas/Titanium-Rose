#pragma once
#include "Hazel/Core/Layer.h"

class EmptyLayer: public Hazel::Layer
{
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Hazel::Timestep ts) override;

};

