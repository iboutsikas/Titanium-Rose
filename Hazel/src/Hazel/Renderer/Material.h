#pragma once
#include "glm/vec4.hpp"
#include "Hazel/Core/Core.h"
#include "Platform/D3D12/D3D12Texture.h"

#include "Hazel/Core/Log.h"

namespace Hazel {
	class HMaterial 
	{
	public:
		~HMaterial() 
		{
			HZ_CORE_WARN("Material {0} destroyed", Name);
		}

		float Specular;
		glm::vec4 Color;
		glm::vec4 EmissiveColor;

		bool HasAlbedoTexture;
		bool HasNormalTexture;
		bool HasSpecularTexture;
		bool IsTransparent;

		Hazel::Ref<Hazel::D3D12Texture2D> AlbedoTexture;
		Hazel::Ref<Hazel::D3D12Texture2D> NormalTexture;
		Hazel::Ref<Hazel::D3D12Texture2D> SpecularTexture;
		std::string Name;
	};
#if 0
	class HMaterialInstance 
	{
	public: 
		explicit HMaterialInstance(HMaterial* pMaterial);

		virtual Ref<D3D12Texture2D> GetAlbedoTexture();
		virtual Ref<D3D12Texture2D> GetNormalTexture();
		virtual Ref<D3D12Texture2D> GetSpecularTexture();


	protected:
		enum ModifiedPropertiesFlags 
		{
			ModifiedProperty_AlbedoTexture,
			ModifiedProperty_AlbedoColor,
			ModifiedProperty_EmissiveColor,
			ModifiedProperty_Count
		};
	};
#endif
}