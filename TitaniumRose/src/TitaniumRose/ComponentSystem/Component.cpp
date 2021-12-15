#include "trpch.h"

#include "TitaniumRose/ComponentSystem/Component.h"
#include "TitaniumRose/ComponentSystem/GameObject.h"

namespace Roses
{
    void LightComponent::OnUpdate(Timestep ts)
    {
        HZ_CORE_ASSERT(gameObject->Material != nullptr, "Lights should have a material");

        gameObject->Material->Color = Color;
        gameObject->Material->EmissiveColor = Color;
    }
}