#include "trpch.h"
#include "TitaniumRose/Core/Math/Ray.h"

#include "TitaniumRose/Scene/Scene.h"

namespace Roses
{

    Ray::Ray(glm::vec3& origin, glm::vec3& dir) :
        m_Origin(origin), m_Direction(dir)
    {

    }

    Ray::Ray(glm::vec3&& origin, glm::vec3&& dir)
        : m_Origin(std::move(origin)), m_Direction(std::move(dir))
    {

    }
    
    // [https://gamedev.stackexchange.com/a/18459]
    bool Ray::Intersects(AABB& aabb, float& t) const
    {
        glm::vec3 dirfrac;
        // r.dir is unit direction vector of ray
        dirfrac.x = 1.0f / m_Direction.x;
        dirfrac.y = 1.0f / m_Direction.y;
        dirfrac.z = 1.0f / m_Direction.z;
        // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
        // r.org is origin of ray
        const glm::vec3& lb = aabb.Min;
        const glm::vec3& rt = aabb.Max;
        float t1 = (lb.x - m_Origin.x) * dirfrac.x;
        float t2 = (rt.x - m_Origin.x) * dirfrac.x;
        float t3 = (lb.y - m_Origin.y) * dirfrac.y;
        float t4 = (rt.y - m_Origin.y) * dirfrac.y;
        float t5 = (lb.z - m_Origin.z) * dirfrac.z;
        float t6 = (rt.z - m_Origin.z) * dirfrac.z;

        float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
        float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

        // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
        if (tmax < 0)
        {
            t = tmax;
            return false;
        }

        // if tmin > tmax, ray doesn't intersect AABB
        if (tmin > tmax)
        {
            t = tmax;
            return false;
        }

        t = tmin;
        return true;
    }

    bool Ray::Intersects(const glm::vec3& V0, const glm::vec3& V1, const glm::vec3& V2, float& t) const
    {
        glm::vec3 E1 = V1 - V0;
        glm::vec3 E2 = V2 - V0;
        glm::vec3 N = cross(E1, E2);
        float det = -glm::dot(m_Direction, N);
        float invdet = 1.0 / det;
        glm::vec3 AO = m_Origin -V0;
        glm::vec3 DAO = glm::cross(AO, m_Direction);
        float u = glm::dot(E2, DAO) * invdet;
        float v = -glm::dot(E1, DAO) * invdet;
        t = glm::dot(AO, N) * invdet;
        return (det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u + v) <= 1.0);
    }

    bool Ray::Raycast(Scene& scene, Ray& ray, RaycastHit& hit)
    {
        std::vector<RaycastHit> hits;

        for (auto& e : scene.GetEntities())
        {
            //if (e->Name == "Plane")
            //    __debugbreak();
            Raycast(e, ray, hits);

        }
        
        if (hits.size() > 0) {
            std::sort(hits.begin(), hits.end(), [](auto& a, auto& b) {
                return a.t < b.t;
            });
            hit.GameObject = hits[0].GameObject;
            hit.t = hits[0].t;
            return true;
        }

        return false;
    }

    bool Ray::Raycast(Ref<HGameObject> entity, Ray& ray, std::vector<RaycastHit>& hits)
    {
        if (entity->Mesh == nullptr)
        {
            goto Raycast_Children;
        }
        auto transform = entity->Transform.LocalToWorldMatrix();

        Ray r = {
                glm::inverse(transform) * glm::vec4(ray.m_Origin, 1.0f),
                glm::inverse(glm::mat3(transform)) * ray.m_Direction
        };

        float t;
        auto& mesh = entity->Mesh;
        if (r.Intersects(mesh->BoundingBox, t)) {
            for (auto& tri : mesh->triangles) {
                float tt;
                if (r.Intersects(tri.V0->Position, tri.V1->Position, tri.V2->Position, tt)) {

                    hits.push_back({ entity, tt });
                    break;
                }
            }
        }

 Raycast_Children:
        
        for (auto& c : entity->children) {
            if (Raycast(c, ray, hits)) {
                return true;
            }
        }

        return false;
    }

    
}
