#pragma once

#if 1
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtx/hash.hpp"


namespace Roses
{
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 Binormal;
        glm::vec2 UV;

        Vertex() :
            Position({ 0.0f, 0.0f, 0.0f }),
            Normal({ 1.0f, 1.0f, 1.0f }),
            Tangent({ 1.0f, 1.0f, 1.0f }),
            UV({ 1.0f, 1.0f })
        {}

        Vertex(glm::vec3 position, glm::vec3 normal, glm::vec3 tangent, glm::vec2 uv)
            : Position(position), Normal(normal), Tangent(tangent), UV(uv)
        { }

        Vertex(glm::vec3 position)
            : Position(position), Normal({ 1.0f, 1.0f, 1.0f }), Tangent({ 1.0f, 1.0f, 1.0f }), UV({ 1.0f, 1.0f })
        { }

        bool operator==(const Vertex& other) const {
            return Position == other.Position
                && Normal == other.Normal
                && Tangent == other.Tangent
                && UV == other.UV;
        }
    };
}

namespace std {
    template<> struct hash<Roses::Vertex> {
        size_t operator()(Roses::Vertex const& vertex) const {
            return (hash<glm::vec3>()(vertex.Position) ^
                (hash<glm::vec3>()(vertex.Normal) << 2) >> 2 ^
                (hash<glm::vec2>()(vertex.UV) << 1));
        }
    };
}
#endif