#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/ShaderLibrary.h"
#include "Hazel/Renderer/TextureLibrary.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Context.h"

#include "glm/vec4.hpp"

#include "d3d12.h"



namespace Hazel
{

    class D3D12Renderer
    {
    public:

        enum RendererType 
        {
            RendererType_Forward,
            RendererType_TextureSpace,
            RendererType_Count
        };

        static ShaderLibrary* ShaderLibrary;
        static TextureLibrary* TextureLibrary;
        static D3D12Context* Context;

        /// <summary>
        /// This method will transition the current back buffer to a render target, and clear it.
        /// The commands are executed immediately, and the thread blocks until it is finished.
        /// </summary>
        /// <param name="clear"></param>
        static void PrepareBackBuffer(glm::vec4 clear = { 0, 0, 0, 0 });
        
        /// <summary>
        /// This method will prepare the resources for a new frame.
        /// It deals with pointing to the correct back buffer, etc.
        /// Does not execute any commands on the gpu;
        /// </summary>
        static void NewFrame();

        /// <summary>
        /// This will swap the back buffers and execute the "main" command list
        /// All the rendering should have occured in previous command lists ideally.
        /// The rendering of ImGui also happens here.
        /// </summary>
        static void Present();

        static void ResizeViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        static void SetVCsync(bool enable);

        static void AddStaticResource(Ref<D3D12Texture2D> texture);
        static void AddStaticRenderTarget(Ref<D3D12Texture2D> texture);

        static void Submit(Ref<GameObject>& gameObject);
        static void RenderSubmitted(Scene& scene);

        static RendererType GetCurrentRenderType() { return s_CurrentRenderer->ImplGetRendererType(); }

        static void Init();
        static void Shutdown();
    protected:

        static D3D12_INPUT_ELEMENT_DESC s_InputLayout[];
        static uint32_t s_InputLayoutCount;

        struct CommonData 
        {
            size_t StaticResources;
            size_t StaticRenderTargets;
        };

        static D3D12DescriptorHeap* ResourceDescriptorHeap;
        static D3D12DescriptorHeap* RenderTargetDescriptorHeap;

        static std::vector<Ref<GameObject>> s_OpaqueObjects;
        static std::vector<Ref<GameObject>> s_TransparentObjects;
        static std::vector<D3D12Renderer*> s_AvailableRenderers;

        static D3D12Renderer* s_CurrentRenderer;

        virtual void ImplRenderSubmitted(Scene& scene) = 0;
        virtual void ImplOnInit() = 0;
        virtual RendererType ImplGetRendererType() = 0;
    };
}


