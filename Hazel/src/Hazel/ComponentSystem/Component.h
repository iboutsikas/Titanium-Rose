#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"

#include "glm/vec3.hpp"




//static constexpr std::string& GetID() { return #type; }


namespace Hazel {

    class GameObject;

    typedef size_t ComponentID;


    template <ComponentID N>
    constexpr inline ComponentID HORNER_HASH(size_t prime, const char(&str)[N], ComponentID Len = N - 1)
    {
        return (Len <= 1) ? str[0] : (prime * HORNER_HASH(prime, str, Len - 1) + str[Len - 1]);
    }

#define DEFINE_COMPONENT_ID(type) \
static constexpr Hazel::ComponentID ID  = Hazel::HORNER_HASH(31, #type);\
inline Hazel::ComponentID GetTypeID() override { return ID; }

    class Component
    {
    public:

        GameObject* gameObject;

        virtual void OnUpdate(Timestep ts) {}
        virtual void OnEvent(Event& e) {}
        virtual ComponentID GetTypeID() = 0;
    };


    class LightComponent: public Component
    {
    public:
        DEFINE_COMPONENT_ID(LightComponent);

        int32_t Range;
        float Intensity;
        glm::vec3 Color;

        virtual void OnUpdate(Timestep ts) override;

    };

};