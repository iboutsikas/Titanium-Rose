#pragma once
#include "glm/vec4.hpp"
#include "Hazel/Core/Core.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel {
	struct HMaterial {
		float Glossines;
		glm::vec4 Color;
		uint32_t cbIndex;
		Hazel::Ref<Hazel::D3D12Texture2D> AlbedoTexture;
		Hazel::Ref<Hazel::D3D12Texture2D> NormalTexture;
	};
}