#include "hzpch.h"
#include "Hazel/ComponentSystem/GameObject.h"

void Hazel::HGameObject::Update(Timestep ts)
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
