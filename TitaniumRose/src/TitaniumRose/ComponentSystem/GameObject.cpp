#include "trpch.h"
#include "TitaniumRose/ComponentSystem/GameObject.h"

void Roses::HGameObject::Update(Timestep ts)
{
    for (auto& c : Components)
    {
        c->OnUpdate(ts);
    }

    for (auto child : children)
    {
        child->Update(ts);
    }
}
