#pragma once
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel
{
    class D3D12Shader;

    class DecoupledRenderer : public D3D12Renderer
    {
    public:
        void ImplRenderVirtualTextures();
        void ImplDilateVirtualTextures();
        
        Ref<D3D12Shader> GetShader();

    protected:
        virtual void ImplRenderSubmitted() override;
        virtual void ImplOnInit() override;
        virtual void ImplOnFrameEnd() override;
        virtual void ImplSubmit(Ref<GameObject> gameObject) override {};
        virtual void ImplSubmit(D3D12ResourceBatch& batch, Ref<GameObject> gameObject) override;
        virtual RendererType ImplGetRendererType() override { return D3D12Renderer::RendererType_TextureSpace; }


    private:
        static constexpr uint32_t MaxItemsPerQueue = 25;
        
        enum ShaderIndices
        {
            ShaderIndices_Albedo,
            ShaderIndices_Normal,
            ShaderIndices_Metalness,
            ShaderIndices_Roughness,
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
            uint32_t FinestMip;
            glm::ivec3 _Padding;
        };

        struct alignas(16) HPassData {
            glm::mat4 ViewProjection;
            glm::vec3 EyePosition;
            uint32_t NumLights;
        };

        enum ShaderIndicesSimple
        {
            ShaderIndicesSimple_Color,
            ShaderIndicesSimple_FeedbackMap,
            ShaderIndicesSimple_PerObject,
            ShaderIndicesSimple_Pass,
            ShaderIndicesSimple_Count
        };

        struct alignas(16) HPassDataSimple {
            glm::mat4 ViewProjection;
        };

        struct alignas(16) HPerObjectDataSimple {
            glm::mat4 LocalToWorld;
            glm::ivec2 FeedbackDims;
            glm::ivec2 _padding;
        };

        struct DilateTextureInfo 
        {
            Ref<D3D12VirtualTexture2D> Temporary;
            Ref<D3D12VirtualTexture2D> Target;
        };


    private:
        std::vector<DilateTextureInfo> m_DilationQueue;
    };
}
