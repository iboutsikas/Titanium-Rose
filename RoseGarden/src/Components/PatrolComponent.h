#pragma once

#include "TitaniumRose/ComponentSystem/Component.h"
#include "TitaniumRose/ComponentSystem/GameObject.h"
#include "glm/vec3.hpp"


struct Waypoint
{
    glm::vec3 Position;
    Waypoint* Next;
};

class PatrolComponent : public Roses::Component
{
public:

    DEFINE_COMPONENT_ID(PatrolComponent)


    bool Patrol = false;
    Waypoint* NextWaypoint = nullptr;
    float Speed = 5.0f;

    virtual void OnUpdate(Roses::Timestep ts) override
    {
        if (!Patrol || gameObject == nullptr || NextWaypoint == nullptr)
        {
            return;
        }

        glm::vec3 pos = gameObject->Transform.Position();
        glm::vec3 movementVector = NextWaypoint->Position - pos;

        float displacement = Speed * ts.GetSeconds();

        if (glm::length2(movementVector) > displacement * displacement)
        {
            // we move towards nextwaypoint
            glm::vec3 direction = glm::normalize(movementVector);
            glm::vec3 target_pos = pos + direction * displacement;

            gameObject->Transform.SetPosition(target_pos);
        }
        else
        {
            // we update the waypoint
            NextWaypoint = NextWaypoint->Next;
        }
    }

};

extern void RenderPatrolComponent(Roses::Ref<Roses::Component> comp);
