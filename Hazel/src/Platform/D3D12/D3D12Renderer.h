#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/ShaderLibrary.h"
#include "Hazel/Renderer/TextureLibrary.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/FrameBuffer.h"

#include "glm/vec4.hpp"

#include "d3d12.h"



namespace Hazel
{
    class D3D12VertexBuffer;
    class D3D12IndexBuffer;

    class D3D12Renderer
    {
    public:

        static constexpr uint8_t MaxSupportedLights = 25;
        static constexpr uint8_t MipsPerIteration = 4;

        enum RendererType 
        {
            RendererType_Forward,
            RendererType_TextureSpace,
            RendererType_Count
        };

        static ShaderLibrary* ShaderLibrary;
        static TextureLibrary* TextureLibrary;
        static D3D12Context* Context;

        static D3D12DescriptorHeap* s_ResourceDescriptorHeap;
        static D3D12DescriptorHeap* s_RenderTargetDescriptorHeap;
        static D3D12DescriptorHeap* s_DepthStencilDescriptorHeap;


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


        static void BeginScene(Scene& scene);
        static void EndScene();

        static void ResizeViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        static void SetVCsync(bool enable);

        static void AddStaticResource(Ref<D3D12Texture> texture);
        static void AddDynamicResource(Ref<D3D12Texture> texture);
        static void ReleaseDynamicResource(Ref<D3D12Texture> texture);
        static void AddStaticRenderTarget(Ref<D3D12Texture> texture);

        static void Submit(Ref<GameObject>& gameObject);
        static void RenderSubmitted();
        static void RenderSkybox(uint32_t mipLevel = 0);
        static void DoToneMapping();

        static void GenerateMips(Ref<D3D12Texture>& texture, uint32_t mostDetailedMip = 0);

        static RendererType GetCurrentRenderType() { return s_CurrentRenderer->ImplGetRendererType(); }

        static std::pair<Ref<D3D12TextureCube>, Ref<D3D12TextureCube>> LoadEnvironmentMap(std::string& path);
        static Ref<FrameBuffer> ResolveFrameBuffer();

        static void CreateUAV(Ref<D3D12Texture>& texture, uint32_t mip);
        static void CreateUAV(Ref<D3D12Texture>& texture, HeapAllocationDescription& description, uint32_t mip);
        static void CreateSRV(Ref<D3D12Texture>& texture, uint32_t mostDetailedMip = 0, uint32_t mips = 0, bool forceArray = false);
        static void CreateSRV(Ref<D3D12Texture>& texture, HeapAllocationDescription& description, uint32_t mostDetailedMip = 0, uint32_t mips = 0, bool forceArray = false);
        static void CreateRTV(Ref<D3D12Texture>& texture, uint32_t mip = 0);
        static void CreateRTV(Ref<D3D12Texture>& texture, HeapAllocationDescription& description, uint32_t mip = 0);
        //static void CreateDSV(Ref<D3D12Texture>& texture);
        static void CreateDSV(Ref<D3D12Texture>& texture, HeapAllocationDescription& description);

        static inline HeapAllocationDescription GetImguiAllocation() { return s_ImGuiAllocation; }

        static void Init();
        static void Shutdown();
    protected:

        

        struct CommonData 
        {
            CommonData() : 
                StaticResources(0), DynamicResources(0),
                StaticRenderTargets(0), DynamicRenderTargets(0),
                NumLights(0),
                Scene(nullptr)
            {
                
            }

            size_t StaticResources;
            size_t DynamicResources;
            size_t StaticRenderTargets;
            size_t DynamicRenderTargets;
            size_t NumLights;
            Scene* Scene;
        };

        static CommonData s_CommonData;

        struct alignas(16) RendererLight
        {
            glm::vec4 Position;
            // ( 16 bytes )
            glm::vec3 Color;
            float Intensity;
            // ( 16 bytes )
            float Range;
            float _padding[3];
            // ( 16 bytes )
        };

        static D3D12_INPUT_ELEMENT_DESC s_InputLayout[];
        static uint32_t s_InputLayoutCount;
        static uint32_t s_CurrentFrameBuffer;

        static std::vector<Ref<GameObject>> s_OpaqueObjects;
        static std::vector<Ref<GameObject>> s_TransparentObjects;
        static std::vector<D3D12Renderer*> s_AvailableRenderers;
        static std::vector<Ref<FrameBuffer>> s_Framebuffers;
        static Scope<D3D12UploadBuffer<RendererLight>> s_LightsBuffer;
        static HeapAllocationDescription s_LightsBufferAllocation;
        static HeapAllocationDescription s_ImGuiAllocation;
        static D3D12Renderer* s_CurrentRenderer;
        static D3D12VertexBuffer* s_FullscreenQuadVB;
        static D3D12IndexBuffer* s_FullscreenQuadIB;

        static void ReclaimDynamicDescriptors();
        static void CreateFrameBuffers();
        static void CreateFrameBuffers(D3D12ResourceBatch& batch);




        virtual void ImplRenderSubmitted() = 0;
        virtual void ImplOnInit() = 0;
        virtual void ImplSubmit(Ref<GameObject>& gameObject) {};
        virtual RendererType ImplGetRendererType() = 0;
    };
}


