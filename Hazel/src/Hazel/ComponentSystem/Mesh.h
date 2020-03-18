#pragma once

#include "Hazel.h"
#include "Platform/D3D12/D3D12Buffer.h"

#include "tinygltf/tiny_gltf.h"
#include <stdint.h>

class MeshData 
{
public:
	struct BufferInfo {
		uint8_t* Data;
		uint32_t Stride;
		uint32_t Size;

		bool HasData() const noexcept {
			return Data != nullptr;
		}
	};

	
};

class Mesh
{
public:

	Mesh(tinygltf::Model& scene, size_t meshIndex) {};

	Hazel::Ref<Hazel::D3D12VertexBuffer> m_VertexBuffer;
	Hazel::Ref<Hazel::D3D12IndexBuffer> m_IndexBuffer;
};

