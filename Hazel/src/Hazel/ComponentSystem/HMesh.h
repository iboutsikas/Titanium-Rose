#pragma once

#include "Platform/D3D12/D3D12Buffer.h"



namespace Hazel {
	struct HMesh
	{
		Hazel::Ref<Hazel::D3D12VertexBuffer> vertexBuffer;
		Hazel::Ref<Hazel::D3D12IndexBuffer> indexBuffer;


		static Hazel::Ref<Hazel::HMesh> BuildFromSomething();
	};
}

