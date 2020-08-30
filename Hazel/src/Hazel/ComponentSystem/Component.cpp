#include "hzpch.h"

#include "Hazel/ComponentSystem/Component.h"
#include "Hazel/ComponentSystem/GameObject.h"

namespace Hazel
{
    void LightComponent::OnUpdate(Timestep ts)
    {
        HZ_CORE_ASSERT(gameObject->Material != nullptr, "Lights should have a material");

        gameObject->Material->Color = Color;
        gameObject->Material->EmissiveColor = Color;
    }
}