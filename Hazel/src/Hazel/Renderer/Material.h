#pragma once
#include "glm/vec4.hpp"
#include "Hazel/Core/Core.h"
#include "Platform/D3D12/D3D12Texture.h"

#include "Hazel/Core/Log.h"

namespace Hazel {
	struct HMaterial {

		~HMaterial() 
		{
			HZ_CORE_WARN("Material {0} destroyed", Name);
		}

		float Specular;
		glm::vec4 Color;

		bool HasAlbedoTexture;
		bool HasNormalTexture;
		bool HasSpecularTexture;
		bool IsTransparent;

		uint32_t cbIndex;
		Hazel::Ref<Hazel::D3D12Texture2D> AlbedoTexture;
		Hazel::Ref<Hazel::D3D12Texture2D> NormalTexture;
		Hazel::Ref<Hazel::D3D12Texture2D> SpecularTexture;
		std::string Name;
	};
}