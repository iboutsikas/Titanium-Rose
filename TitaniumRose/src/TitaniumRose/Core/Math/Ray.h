#pragma once
#include "glm/vec3.hpp"
#include "TitaniumRose/ComponentSystem/GameObject.h"
#include "TitaniumRose/Core/Math/AABB.h"

namespace Roses
{
    class Scene;

    struct RaycastHit
    {
        Ref<HGameObject> GameObject;
        float t;
    };

    class Ray
    {
    public:
        Ray(glm::vec3& orign, glm::vec3& dir);
        Ray(glm::vec3&& orign, glm::vec3&& dir);

        bool Intersects(AABB& aabb, float& t) const;
        bool Intersects(const glm::vec3& V0, const glm::vec3& V1, const glm::vec3& V2, float& t) const;

        static bool Raycast(Scene& scene, Ray& ray, RaycastHit& hit);

    private:
        static bool Raycast(Ref<HGameObject>, Ray& ray, std::vector<RaycastHit>& hits);
        glm::vec3 m_Origin;
        glm::vec3 m_Direction;
    };

}

