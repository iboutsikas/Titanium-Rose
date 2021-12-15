#pragma once

#include "TitaniumRose/Core/Math/AABB.h"
#include "TitaniumRose/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"

namespace Roses {
	struct Triangle
	{
        Vertex* V0, * V1, * V2;
	};

	struct HMesh
	{
		Roses::Ref<Roses::D3D12VertexBuffer> vertexBuffer;
		Roses::Ref<Roses::D3D12IndexBuffer> indexBuffer;

		std::vector<Roses::Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Triangle> triangles;

		AABB BoundingBox;
	};
}
