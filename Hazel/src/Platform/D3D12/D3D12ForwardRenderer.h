#pragma once
#include "Platform/D3D12/D3D12Renderer.h"

namespace Hazel
{
    class D3D12ForwardRenderer: public D3D12Renderer
    {
    public:
		static constexpr uint8_t MaxSupportedLights = 2;

    protected:
        virtual void ImplRenderSubmitted() override;
		virtual void ImplOnInit() override;
        virtual RendererType ImplGetRendererType() override { return RendererType_Forward; }

    private:
		static constexpr uint32_t MaxItemsPerQueue = 25;

        enum ShaderIndices
        {
            ShaderIndices_Albedo,
            ShaderIndices_Normal,
            ShaderIndices_Specular,
			ShaderIndices_Lights,
            ShaderIndices_PerObject,
            ShaderIndices_Pass,
            ShaderIndices_Count
        };

		struct alignas(16) HPerObjectData {
			glm::mat4 LocalToWorld;
			glm::mat4 WorldToLocal;
			glm::vec4 Color;
			glm::vec4 EmissiveColor;
			float Specular;
			uint32_t HasAlbedo;
			uint32_t HasNormal;
			uint32_t HasSpecular;
		};

		struct alignas(16) HPassData {
			glm::mat4 ViewProjection;
			glm::vec3 AmbientLight;
			float AmbientIntensity;
			glm::vec3 EyePosition;
			uint32_t NumLights;
		};
        
    };
}


