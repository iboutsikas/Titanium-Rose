#pragma once
#include "Platform/D3D12/D3D12Renderer.h"

namespace Hazel
{
    class D3D12ForwardRenderer : public D3D12Renderer
    {
    public:
        /*static constexpr uint8_t MaxSupportedLights = 2;*/

    protected:
        virtual void ImplRenderSubmitted(GraphicsContext& gfxContext) override;
        virtual void ImplOnInit() override;
        virtual void ImplSubmit(Ref<HGameObject> gameObject) override;
        virtual void ImplSubmit(D3D12ResourceBatch& batch, Ref<HGameObject> gameObject) override {};
        virtual RendererType ImplGetRendererType() override { return RendererType_Forward; }


    private:
        static constexpr uint32_t MaxItemsPerQueue = 25;

        enum ShaderIndices
        {
            ShaderIndices_Albedo,
            ShaderIndices_Normal,
            ShaderIndices_Metalness,
            ShaderIndices_Roughness,
            ShaderIndices_ObjectLightsList,
            ShaderIndices_EnvRadiance,
            ShaderIndices_EnvIrradiance,
            ShaderIndices_BRDFLUT,
            ShaderIndices_Lights,
            ShaderIndices_PerObject,
            ShaderIndices_Pass,
            ShaderIndices_Count
        };

        struct alignas(16) HPerObjectData {
            glm::mat4 LocalToWorld;
            // ----- 16 bytes -----
            glm::mat4 WorldToLocal;
            // ----- 16 bytes -----
            glm::vec3 MaterialColor;
            uint32_t HasAlbedo;
            // ----- 16 bytes -----
            glm::vec3 EmissiveColor;
            uint32_t HasNormal;
            // ----- 16 bytes -----
            uint32_t HasMetallic;
            float Metallic;
            uint32_t HasRoughness;
            float Roughness;
            // ----- 16 bytes -----
            uint32_t NumObjectLights;
            uint32_t _padding[3];
            //// ----- 16 bytes -----
        };

        struct alignas(16) HPassData {
            glm::mat4 ViewProjection;
            glm::vec3 EyePosition;
            uint32_t NumLights;
        };


    };
}
