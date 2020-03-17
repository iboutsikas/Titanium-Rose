#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

struct Vertex {
	glm::vec3 Position;
	glm::vec4 Color;
	glm::vec3 Normal;
	glm::vec2 UV;

	Vertex(glm::vec3 position, glm::vec4 color, glm::vec3 normal, glm::vec2 uv)
		: Position(position), Color(color), Normal(normal), UV(uv)
	{ }

	Vertex(glm::vec3 position)
		: Position(position), Color({ 1.0f, 1.0f, 1.0f, 1.0f }), Normal({ 1.0f, 1.0f, 1.0f }), UV({ 1.0f, 1.0f })
	{ }

	bool operator==(const Vertex& other) const {
		return Position == other.Position
			&& Color == other.Color
			&& Normal == other.Normal
			&& UV == other.UV;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.Position) ^
				(hash<glm::vec4>()(vertex.Color) << 1)) >> 1) ^
				((hash<glm::vec3>()(vertex.Normal) << 2)) >> 2 ^
				(hash<glm::vec2>()(vertex.UV) << 1);
		}
	};
}