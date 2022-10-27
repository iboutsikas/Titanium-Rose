#include "trpch.h"
#include "Platform/D3D12/DecoupledRenderer.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/TextureManager.h"

#include "winpixeventruntime/pix3.h"

namespace Roses
{
    DECLARE_SHADER_NAMED("SurfaceShader-Decoupled", Decoupled);
    DECLARE_SHADER_NAMED("SurfaceShader-Simple", Simple);
    DECLARE_SHADER(Dilate);

    // 
    void DecoupledRenderer::ImplRenderVirtualTextures(GraphicsContext& gfxContext)
    {
        auto shader = GetShader();
        auto envRad = s_CommonData.Scene->Environment.EnvironmentMap;
        auto envIrr = s_CommonData.Scene->Environment.IrradianceMap;
        auto lut = g_TextureLibrary->GetAs<Texture2D>(std::string("spbrdf"));

        HPassData passData;
        passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();
        passData.NumLights = s_CommonData.NumLights;
        passData.EyePosition = s_CommonData.Scene->Camera->GetPosition();

        //ScopedTimer passTimer("Virtual Render", commandList);

        gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
        gfxContext.GetCommandList()->SetGraphicsRootSignature(shader->GetRootSignature());
        gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        gfxContext.SetDynamicContantBufferView(ShaderIndices_Pass, sizeof(HPassData), &passData);
        gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Lights, s_LightsBufferAllocation.GPUHandle);
        gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_EnvRadiance, envRad->SRVAllocation.GPUHandle);
        gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_EnvIrradiance, envIrr->SRVAllocation.GPUHandle);
        gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_BRDFLUT, lut->SRVAllocation.GPUHandle);


        for (uint32_t i = 0; i < s_DecoupledOpaqueObjects.size(); i++)
        {
            auto obj = s_DecoupledOpaqueObjects[i];
            s_SimpleOpaqueObjects.push_back(obj);

            if (obj == nullptr) {
                continue;
            }

            if (obj->Mesh == nullptr) {
                continue;
            }
            auto virtualTexture = obj->DecoupledComponent.VirtualTexture;

            std::vector<uint32_t> lights;

            for (uint32_t l = 0; l < s_CommonData.Scene->Lights.size(); l++)
            {
#if 0
                lights.push_back(i);
#else
                auto light = s_CommonData.Scene->Lights[l];
                
                auto v = light->gameObject->Transform.Position() - obj->Transform.Position();

                auto d = glm::length(v);

                if (d <= light->Range * 2) {
                    lights.push_back(l);
                }
#endif           
            }

            if (!lights.empty())
            {
                D3D12UploadBuffer<uint32_t>* objectLightsList = new D3D12UploadBuffer<uint32_t>(
                    lights.size(),
                    false
                    );
                objectLightsList->CopyDataBlock(lights.size(), lights.data());
                CreateBufferSRV(*objectLightsList, lights.size(), sizeof(uint32_t));
                gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_ObjectLightsList, objectLightsList->SRVAllocation.GPUHandle);
                gfxContext.TrackResource(objectLightsList);
                gfxContext.TrackAllocation(objectLightsList->SRVAllocation);
            }

            ScopedTimer timer(obj->Name, gfxContext);

            auto mips = virtualTexture->GetMipsUsed();

            auto targetWidth = virtualTexture->GetWidth() >> mips.FinestMip;
            auto targetHeight = virtualTexture->GetHeight() >> mips.FinestMip;
            auto targetFormat = virtualTexture->GetFormat();

            Texture2D* temporaryRenderTarget = TextureManager::RequestTemporaryRenderTarget(targetWidth, targetHeight, targetFormat, gfxContext.GetCompletedFenceValue());
            temporaryRenderTarget->SetName(virtualTexture->GetIdentifier() + ".temp");
            
            gfxContext.TransitionResource(*temporaryRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

            //TilePool->MapTexture(*temporaryRenderTarget);

            DilateTextureInfo dilateTextureInfo = {};
            dilateTextureInfo.Target = virtualTexture;
            dilateTextureInfo.Temporary = temporaryRenderTarget;

            CreateRTV(*temporaryRenderTarget, 0);
            gfxContext.TrackAllocation(temporaryRenderTarget->RTVAllocation, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            m_DilationQueue.emplace_back(dilateTextureInfo);

            D3D12_VIEWPORT vp = { 0, 0, targetWidth, targetHeight, 0, 1 };
            D3D12_RECT rect = { 0, 0, targetWidth, targetHeight };

            gfxContext.GetCommandList()->RSSetViewports(1, &vp);
            gfxContext.GetCommandList()->RSSetScissorRects(1, &rect);
            gfxContext.GetCommandList()->OMSetRenderTargets(1, &temporaryRenderTarget->RTVAllocation.CPUHandle, true, nullptr);
            static const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            gfxContext.GetCommandList()->ClearRenderTargetView(temporaryRenderTarget->RTVAllocation.CPUHandle, clearColor, 0, nullptr);

            HPerObjectData objectData;
            objectData.LocalToWorld = obj->Transform.LocalToWorldMatrix();
            objectData.WorldToLocal = glm::transpose(obj->Transform.WorldToLocalMatrix());
            objectData.MaterialColor = obj->Material->Color;
            objectData.HasAlbedo = obj->Material->HasAlbedoTexture;
            objectData.EmissiveColor = obj->Material->EmissiveColor;
            objectData.HasNormal = obj->Material->HasNormalTexture;
            objectData.HasMetallic = obj->Material->HasMetallicTexture;
            objectData.Metallic = obj->Material->Metallic;
            objectData.HasRoughness = obj->Material->HasRoughnessTexture;
            objectData.Roughness = obj->Material->Roughness;
            objectData.FinestMip = mips.FinestMip;
            objectData.NumObjectLights = lights.size();
            

            auto vb = obj->Mesh->vertexBuffer->GetView();
            vb.StrideInBytes = sizeof(Vertex);
            auto ib = obj->Mesh->indexBuffer->GetView();

            gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &vb);
            gfxContext.GetCommandList()->IASetIndexBuffer(&ib);
            if (obj->Material->HasAlbedoTexture) {
                gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Albedo, obj->Material->AlbedoTexture->SRVAllocation.GPUHandle);
            }

            if (obj->Material->HasNormalTexture) {
                gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Normal, obj->Material->NormalTexture->SRVAllocation.GPUHandle);
            }

            if (obj->Material->HasRoughnessTexture) {
                gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Roughness, obj->Material->RoughnessTexture->SRVAllocation.GPUHandle);
            }

            if (obj->Material->HasMetallicTexture) {
                gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Metalness, obj->Material->MetallicTexture->SRVAllocation.GPUHandle);
            }

            gfxContext.SetDynamicContantBufferView(ShaderIndices_PerObject, sizeof(objectData), &objectData);
            gfxContext.GetCommandList()->DrawIndexedInstanced(obj->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
        }
    }

    void DecoupledRenderer::ImplDilateVirtualTextures(GraphicsContext& gfxContext)
    {
        auto shader = g_ShaderLibrary->GetAs<D3D12Shader>(ShaderNameDilate);

        gfxContext.GetCommandList()->SetComputeRootSignature(shader->GetRootSignature());
        gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
        
        for (auto& info : m_DilationQueue)
        {
            //ScopedTimer t(info.Target->GetIdentifier(), commandList);
            auto width = info.Temporary->GetWidth();
            auto height = info.Temporary->GetHeight();
            auto mips = info.Target->GetMipsUsed();

            HZ_CORE_ASSERT(width == (info.Target->GetWidth() >> mips.FinestMip), "Width miss match");
            HZ_CORE_ASSERT(height == (info.Target->GetHeight() >> mips.FinestMip), "Height miss match");

            gfxContext.TransitionResource(*info.Temporary, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            gfxContext.TransitionResource(*info.Target, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            CreateSRV(*(info.Temporary));
            gfxContext.TrackAllocation(info.Temporary->SRVAllocation);

            CreateUAV(info.Target, mips.FinestMip);
            gfxContext.TrackAllocation(info.Target->UAVAllocation);

            glm::vec4 dims = { width, height, 1.0f / width, 1.0f / height };
            gfxContext.GetCommandList()->SetComputeRoot32BitConstants(0, sizeof(dims) / sizeof(float), &dims[0], 0);
            gfxContext.GetCommandList()->SetComputeRootDescriptorTable(1, info.Temporary->SRVAllocation.GPUHandle);
            gfxContext.GetCommandList()->SetComputeRootDescriptorTable(2, info.Target->UAVAllocation.GPUHandle);

            auto x_count = D3D12::RoundToMultiple(width, 8);
            auto y_count = D3D12::RoundToMultiple(height, 8);
            gfxContext.GetCommandList()->Dispatch(x_count, y_count, 1);
            gfxContext.GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(info.Target->GetResource()));
        }
        gfxContext.FlushResourceBarriers();
    }

    Ref<D3D12Shader> DecoupledRenderer::GetShader()
    {
        return g_ShaderLibrary->GetAs<D3D12Shader>(ShaderNameDecoupled);
    }

    void DecoupledRenderer::ImplRenderSubmitted(GraphicsContext& gfxContext)
    {
        if (s_SimpleOpaqueObjects.empty())
            return;

        
        auto shader = g_ShaderLibrary->GetAs<D3D12Shader>(ShaderNameSimple);
        auto framebuffer = ResolveFrameBuffer();

        HPassDataSimple passData;
        passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();


        ScopedTimer timer("Simple Pass", gfxContext);

        gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
        gfxContext.GetCommandList()->SetGraphicsRootSignature(shader->GetRootSignature());
        gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.GetCommandList()->RSSetViewports(1, &Context->Viewport);
        gfxContext.GetCommandList()->RSSetScissorRects(1, &Context->ScissorRect);
        gfxContext.GetCommandList()->OMSetRenderTargets(1, &framebuffer->RTVAllocation.CPUHandle, true, &framebuffer->DSVAllocation.CPUHandle);
        gfxContext.SetDynamicContantBufferView(ShaderIndicesSimple_Pass, sizeof(passData), &passData);

        for (size_t i = 0; i < s_SimpleOpaqueObjects.size(); i++)
        {
            auto go = s_SimpleOpaqueObjects[i];

            if (go == nullptr) {
                continue;
            }

            if (go->Mesh == nullptr) {
                continue;
            }

            ScopedTimer timer(go->Name, gfxContext);

            auto tex = go->DecoupledComponent.VirtualTexture;


            //ScopedTimer timer(tex->GetIdentifier(), commandList);

            gfxContext.TransitionResource(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

            auto fm = tex->GetFeedbackMap();

            CreateSRV(*tex, 0);


            HPerObjectDataSimple objectData;
            objectData.LocalToWorld = go->Transform.LocalToWorldMatrix();
            objectData.FeedbackDims = fm->GetDimensions();
            objectData.Mips = glm::ivec2(tex->GetMipsUsed().CoarsestMip, tex->GetMipsUsed().FinestMip);
            //objectData.EntityID = go->ID;

            auto vb = go->Mesh->vertexBuffer->GetView();
            vb.StrideInBytes = sizeof(Vertex);
            auto ib = go->Mesh->indexBuffer->GetView();

            gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &vb);
            gfxContext.GetCommandList()->IASetIndexBuffer(&ib);

            gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndicesSimple_Color, tex->SRVAllocation.GPUHandle);
            gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndicesSimple_FeedbackMap, fm->UAVAllocation.GPUHandle);
            gfxContext.SetDynamicContantBufferView(ShaderIndicesSimple_PerObject, sizeof(objectData), &objectData);

            gfxContext.GetCommandList()->DrawIndexedInstanced(go->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
        }

        //Profiler::EndBlock(commandList);
    }

    void DecoupledRenderer::ImplOnInit()
    {
        {
            D3D12_RT_FORMAT_ARRAY rtvFormats = {};
            rtvFormats.NumRenderTargets = 1;
            rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

            CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
            rasterizer.FrontCounterClockwise = TRUE;
            rasterizer.CullMode = D3D12_CULL_MODE_NONE;

            CD3DX12_PIPELINE_STATE_STREAM pipelineStateStream;

            pipelineStateStream.InputLayout = { s_InputLayout, s_InputLayoutCount };
            pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            pipelineStateStream.RTVFormats = rtvFormats;
            pipelineStateStream.RasterizerState = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

            CD3DX12_DEPTH_STENCIL_DESC1 depthDesc(D3D12_DEFAULT);
            depthDesc.DepthEnable = false;

            pipelineStateStream.DepthStencilState = CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1(depthDesc);

            auto shader = Roses::CreateRef<D3D12Shader>(ShaderPathDecoupled, pipelineStateStream);
            D3D12Renderer::g_ShaderLibrary->Add(shader);
        }

        {
            D3D12_RT_FORMAT_ARRAY rtvFormats = {};
            rtvFormats.NumRenderTargets = 1;
            rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

            CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
            rasterizer.FrontCounterClockwise = TRUE;
            rasterizer.CullMode = D3D12_CULL_MODE_BACK;

            CD3DX12_PIPELINE_STATE_STREAM pipelineStateStream;

            pipelineStateStream.InputLayout = { s_InputLayout, s_InputLayoutCount };
            pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            pipelineStateStream.RTVFormats = rtvFormats;
            pipelineStateStream.RasterizerState = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);


            auto shader = Roses::CreateRef<D3D12Shader>(ShaderPathSimple, pipelineStateStream);
            D3D12Renderer::g_ShaderLibrary->Add(shader);
        }
    }

    void DecoupledRenderer::ImplOnFrameEnd()
    {
        uint64_t fenceValue = D3D12Renderer::CommandQueueManager.GetGraphicsQueue().GetCompletedFenceValue();

        for (auto& info : m_DilationQueue)
        {
            s_RenderTargetDescriptorHeap->Release(info.Temporary->RTVAllocation);
            TextureManager::DiscardTexture(info.Temporary, fenceValue);
        }
        m_DilationQueue.clear();
    }

    void DecoupledRenderer::ImplSubmit(D3D12ResourceBatch& batch, Ref<HGameObject> gameObject)
    {
        if (gameObject->DecoupledComponent.UseDecoupledTexture)
        {
            s_DecoupledOpaqueObjects.push_back(gameObject);
        }

        if (gameObject->DecoupledComponent.VirtualTexture == nullptr)
        {
            uint32_t width, height, mips;
            std::string name;
            auto albedo = gameObject->Material->AlbedoTexture;

            if (albedo != nullptr) 
            {
                width = albedo->GetWidth();
                height = albedo->GetHeight();
                mips = albedo->GetMipLevels();
                name = albedo->GetIdentifier() + "-virtual";
            }
            else
            {
                width = height = 2048;
                mips = D3D12::CalculateMips(width, height);
                name = gameObject->Material->Name + "-virtual";
            }

            HZ_PROFILE_SCOPE("Virtual Texture");
            TextureCreationOptions opts;
            opts.Name = name;
            opts.Width = width;
            opts.Height = height;
            opts.MipLevels = mips;
            opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            auto tex = Roses::Texture2D::CreateVirtualTexture(opts);
            auto fb = Roses::Texture2D::CreateFeedbackMap(*tex);
            tex->SetFeedbackMap(fb);
            gameObject->DecoupledComponent.VirtualTexture = std::dynamic_pointer_cast<VirtualTexture2D>(tex);

            tex->SRVAllocation = s_ResourceDescriptorHeap->Allocate(1);
            tex->RTVAllocation = s_RenderTargetDescriptorHeap->Allocate(1);
            fb->UAVAllocation = s_ResourceDescriptorHeap->Allocate(1);
            auto fbDesc = fb->GetResource()->GetDesc();

            D3D12_UNORDERED_ACCESS_VIEW_DESC fbUAVDesc = {};
            fbUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            fbUAVDesc.Format = fbDesc.Format;
            auto dims = fb->GetDimensions();
            fbUAVDesc.Buffer.NumElements = dims.x * dims.y;
            fbUAVDesc.Buffer.StructureByteStride = sizeof(uint32_t);
            batch.GetDevice()->CreateUnorderedAccessView(
                fb->GetResource(),
                nullptr,
                &fbUAVDesc,
                fb->UAVAllocation.CPUHandle
            );
            CreateSRV(std::static_pointer_cast<Texture>(tex), 0);
            CreateRTV(std::static_pointer_cast<Texture>(tex), 0);
        }

    }
}
