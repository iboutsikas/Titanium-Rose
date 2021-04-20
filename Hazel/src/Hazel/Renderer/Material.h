#pragma once
#include "glm/vec4.hpp"
#include "Hazel/Core/Core.h"
#include "Platform/D3D12/D3D12Texture.h"

#include "Hazel/Core/Log.h"

namespace Hazel {
	class HMaterial 
	{
	public:
		HMaterial();
		
		~HMaterial() 
		{
			HZ_CORE_WARN("Material {0} destroyed", Name);
		}

		glm::vec3 Color;
		glm::vec3 EmissiveColor;
		float Roughness;
		float Metallic;

		bool HasAlbedoTexture;
		bool HasNormalTexture;
		bool HasRoughnessTexture;
		bool HasMatallicTexture;
		bool IsTransparent;

		Hazel::Ref<Hazel::Texture2D> AlbedoTexture;
		Hazel::Ref<Hazel::Texture2D> NormalTexture;
		Hazel::Ref<Hazel::Texture2D> RoughnessTexture;
		Hazel::Ref<Hazel::Texture2D> MetallicTexture;
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