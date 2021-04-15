#include "hzpch.h"
#include "Platform/D3D12/DecoupledRenderer.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "Platform/D3D12/D3D12TilePool.h"

#include "winpixeventruntime/pix3.h"

namespace Hazel
{
    DECLARE_SHADER_NAMED("SurfaceShader-Decoupled", Decoupled);
    DECLARE_SHADER_NAMED("SurfaceShader-Simple", Simple);
    DECLARE_SHADER(Dilate);

    // 
    void DecoupledRenderer::ImplRenderVirtualTextures()
    {
        HeapValidationMark srvMark(s_ResourceDescriptorHeap);

        auto shader = GetShader();
        auto envRad = s_CommonData.Scene->Environment.EnvironmentMap;
        auto envIrr = s_CommonData.Scene->Environment.IrradianceMap;
        auto lut = TextureLibrary->GetAs<D3D12Texture2D>(std::string("spbrdf"));

        HPassData passData;
        passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();
        passData.NumLights = s_CommonData.NumLights;
        passData.EyePosition = s_CommonData.Scene->Camera->GetPosition();

        std::vector<std::shared_future<void>> renderTasks;
        uint32_t counter = 0;

        auto r = Context->DeviceResources.get();

        while (counter < s_DecoupledObjects.size())
        {
            D3D12ResourceBatch batch(r->Device, r->DecoupledCommandList, true);
            Ref<D3D12CommandList> commandList = batch.Begin(Context->CurrentFrameResource->CommandAllocator2);
            auto cmdlist = commandList->Get();
            
            CPUProfileBlock cpuBlock("Virtual Pass::Render");
            batch.TrackBlock(cpuBlock);
            GPUProfileBlock gpuBlock(commandList, "Virtual Pass::Render");
            batch.TrackBlock(gpuBlock);


            D3D12UploadBuffer<HPerObjectData> perObjectBuffer(batch, MaxItemsPerQueue, true);
            D3D12UploadBuffer<HPassData> passBuffer(batch, 1, true);
            passBuffer.CopyData(0, passData);
            ID3D12DescriptorHeap* heaps[] = {
                s_ResourceDescriptorHeap->GetHeap()
            };

            cmdlist->SetPipelineState(shader->GetPipelineState());
            cmdlist->SetGraphicsRootSignature(shader->GetRootSignature());
            cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdlist->SetDescriptorHeaps(_countof(heaps), heaps);
            cmdlist->SetGraphicsRootConstantBufferView(ShaderIndices_Pass, passBuffer.Resource()->GetGPUVirtualAddress());
            cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_Lights, s_LightsBufferAllocation.GPUHandle);
            cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_EnvRadiance, envRad->SRVAllocation.GPUHandle);
            cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_EnvIrradiance, envIrr->SRVAllocation.GPUHandle);
            cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_BRDFLUT, lut->SRVAllocation.GPUHandle);


            for (int i = 0; i < MaxItemsPerQueue && counter < s_DecoupledObjects.size(); i++)
            {
                auto obj = s_DecoupledObjects[i];
                
                if (obj == nullptr) {
                    continue;
                }

                if (obj->Mesh == nullptr) {
                    continue;
                }
                auto tex = obj->DecoupledComponent.VirtualTexture;

                CPUProfileBlock cpuBlock("Virtual Pass::Render::" + tex->GetIdentifier());
                GPUProfileBlock gpuBlock(commandList, "Virtual Pass::Render::" + tex->GetIdentifier());

                tex->Transition(cmdlist, D3D12_RESOURCE_STATE_RENDER_TARGET);
                auto mips = tex->GetMipsUsed();

                auto targetWidth = tex->GetWidth() >> mips.FinestMip;
                auto targetHeight = tex->GetHeight() >> mips.FinestMip;

                D3D12Texture::TextureCreationOptions opts;
                opts.Width = targetWidth;
                opts.Height = targetHeight;
                opts.MipLevels = 1;
                opts.Format = tex->GetFormat();
                opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

                auto temp = D3D12Texture2D::CreateVirtualTexture(batch, opts);
                temp->Transition(cmdlist, D3D12_RESOURCE_STATE_RENDER_TARGET);
                TilePool->MapTexture(batch, temp, Context->DeviceResources->CommandQueue);

                DilateTextureInfo dilateTextureInfo = {};
                dilateTextureInfo.Target = tex;
                dilateTextureInfo.Temporary = std::static_pointer_cast<D3D12VirtualTexture2D>(temp);

                CreateRTV(std::static_pointer_cast<D3D12Texture>(temp));

                m_DilationQueue.emplace_back(dilateTextureInfo);

                D3D12_VIEWPORT vp = { 0, 0, targetWidth, targetHeight, 0, 1 };
                D3D12_RECT rect = { 0, 0, targetWidth, targetHeight };

                cmdlist->RSSetViewports(1, &vp);
                cmdlist->RSSetScissorRects(1, &rect);
                cmdlist->OMSetRenderTargets(1, &temp->RTVAllocation.CPUHandle, true, nullptr);
                static const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
                cmdlist->ClearRenderTargetView(temp->RTVAllocation.CPUHandle, clearColor, 0, nullptr);

                HPerObjectData objectData;
                objectData.LocalToWorld = obj->Transform.LocalToWorldMatrix();
                objectData.WorldToLocal = glm::transpose(obj->Transform.WorldToLocalMatrix());
                objectData.MaterialColor = obj->Material->Color;
                objectData.HasAlbedo = obj->Material->HasAlbedoTexture;
                objectData.EmissiveColor = obj->Material->EmissiveColor;
                objectData.HasNormal = obj->Material->HasNormalTexture;
                objectData.HasMetallic = obj->Material->HasMatallicTexture;
                objectData.Metallic = obj->Material->Metallic;
                objectData.HasRoughness = obj->Material->HasRoughnessTexture;
                objectData.Roughness = obj->Material->Roughness;
                objectData.FinestMip = mips.FinestMip;
                perObjectBuffer.CopyData(i, objectData);

                auto vb = obj->Mesh->vertexBuffer->GetView();
                vb.StrideInBytes = sizeof(Vertex);
                auto ib = obj->Mesh->indexBuffer->GetView();

                cmdlist->IASetVertexBuffers(0, 1, &vb);
                cmdlist->IASetIndexBuffer(&ib);
                if (obj->Material->HasAlbedoTexture) {
                    cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_Albedo, obj->Material->AlbedoTexture->SRVAllocation.GPUHandle);
                }

                if (obj->Material->HasNormalTexture) {
                    cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_Normal, obj->Material->NormalTexture->SRVAllocation.GPUHandle);
                }

                if (obj->Material->HasRoughnessTexture) {
                    cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_Roughness, obj->Material->RoughnessTexture->SRVAllocation.GPUHandle);
                }

                if (obj->Material->HasMatallicTexture) {
                    cmdlist->SetGraphicsRootDescriptorTable(ShaderIndices_Metalness, obj->Material->MetallicTexture->SRVAllocation.GPUHandle);
                }

                cmdlist->SetGraphicsRootConstantBufferView(ShaderIndices_PerObject,
                    (perObjectBuffer.Resource()->GetGPUVirtualAddress() + perObjectBuffer.CalculateOffset(i))
                );

                cmdlist->DrawIndexedInstanced(obj->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
                tex->Transition(cmdlist, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                counter++;
            }
            batch.TrackResource(perObjectBuffer.Resource());
            batch.TrackResource(passBuffer.Resource());
            renderTasks.push_back(std::move(
                batch.EndAsync(Context->DeviceResources->CommandQueue)
            ));
        }

        for (auto& task : renderTasks)
        {
            task.wait();
        }

        renderTasks.clear();
    }

    void DecoupledRenderer::ImplDilateVirtualTextures()
    {
        HeapValidationMark srvMark(s_ResourceDescriptorHeap);

        auto shader = ShaderLibrary->GetAs<D3D12Shader>(ShaderNameDilate);
        auto r = Context->DeviceResources.get();

        D3D12ResourceBatch batch(r->Device, r->DecoupledCommandList);
        auto myCmdList = batch.Begin(Context->CurrentFrameResource->CommandAllocator2);
        auto cmdlist = myCmdList->Get();
        
        CPUProfileBlock cpuBlock("Virtual Pass::Dilate");
        batch.TrackBlock(cpuBlock);
        GPUProfileBlock gpuBlock(myCmdList, "Virtual Pass::Dilate");
        batch.TrackBlock(gpuBlock);


        cmdlist->SetComputeRootSignature(shader->GetRootSignature());
        cmdlist->SetPipelineState(shader->GetPipelineState());
        ID3D12DescriptorHeap* heaps [] = {
            s_ResourceDescriptorHeap->GetHeap()
        };
        cmdlist->SetDescriptorHeaps(_countof(heaps), heaps);


        for (auto& info : m_DilationQueue)
        {
            auto width = info.Temporary->GetWidth();
            auto height = info.Temporary->GetHeight();
            auto mips = info.Target->GetMipsUsed();

            HZ_CORE_ASSERT(width == (info.Target->GetWidth() >> mips.FinestMip), "Width miss match");
            HZ_CORE_ASSERT(height == (info.Target->GetHeight() >> mips.FinestMip), "Height miss match");

            CPUProfileBlock cpuBlock("Virtual Pass::Dilate::" + info.Target->GetIdentifier());
            GPUProfileBlock gpuBlock(myCmdList, "Virtual Pass::Dilate::" + info.Target->GetIdentifier());

            info.Temporary->Transition(batch, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            info.Target->Transition(batch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            CreateSRV(std::static_pointer_cast<D3D12Texture>(info.Temporary));
            CreateUAV(std::static_pointer_cast<D3D12Texture>(info.Target), mips.FinestMip);

            glm::vec4 dims = { width, height, 1.0f / width, 1.0f / height };
            cmdlist->SetComputeRoot32BitConstants(0, sizeof(dims) / sizeof(float), &dims[0], 0);
            cmdlist->SetComputeRootDescriptorTable(1, info.Temporary->SRVAllocation.GPUHandle);
            cmdlist->SetComputeRootDescriptorTable(2, info.Target->UAVAllocation.GPUHandle);

            auto x_count = D3D12::RoundToMultiple(width, 8);
            auto y_count = D3D12::RoundToMultiple(height, 8);
            cmdlist->Dispatch(x_count, y_count, 1);
            cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(info.Target->GetResource()));
        }
        batch.EndAndWait(Context->DeviceResources->CommandQueue);

        // Clean up m_dilationqueue
        for (auto& info : m_DilationQueue)
        {
            s_ResourceDescriptorHeap->Release(info.Temporary->SRVAllocation);
            s_RenderTargetDescriptorHeap->Release(info.Temporary->RTVAllocation);
            s_ResourceDescriptorHeap->Release(info.Target->UAVAllocation);
            TilePool->ReleaseTexture(info.Temporary, Context->DeviceResources->CommandQueue);
        }
    }

    Ref<D3D12Shader> DecoupledRenderer::GetShader()
    {
        return ShaderLibrary->GetAs<D3D12Shader>(ShaderNameDecoupled);
    }

    void DecoupledRenderer::ImplRenderSubmitted()
    {
        return;
        if (s_DecoupledObjects.empty())
            return;

        std::vector<std::future<void>> renderTasks;

        uint32_t counter = 0;

        auto shader = ShaderLibrary->GetAs<D3D12Shader>(ShaderNameSimple);

        HPassDataSimple passData;
        passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();

        auto framebuffer = ResolveFrameBuffer();
        ID3D12DescriptorHeap* const heaps[] = {
            s_ResourceDescriptorHeap->GetHeap()
        };

        D3D12UploadBuffer<HPassDataSimple> passBuffer(1, true);
        passBuffer.CopyData(0, passData);

        
        auto r = Context->DeviceResources.get();

        while (counter < s_DecoupledObjects.size())
        {
            D3D12ResourceBatch batch(r->Device, r->MainCommandList, true);
            auto myCmdList = batch.Begin(Context->CurrentFrameResource->CommandAllocator);
            auto cmdlist = myCmdList->Get();

            CPUProfileBlock cpuBlock("Simple pass::");
            batch.TrackBlock(cpuBlock);
            GPUProfileBlock gpuBlock(myCmdList, "Simple pass::");
            batch.TrackBlock(gpuBlock);
                        
            D3D12UploadBuffer<HPerObjectDataSimple> perObjectBuffer(batch, MaxItemsPerQueue, true);

            cmdlist->SetPipelineState(shader->GetPipelineState());
            cmdlist->SetGraphicsRootSignature(shader->GetRootSignature());
            cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdlist->RSSetViewports(1, &Context->Viewport);
            cmdlist->RSSetScissorRects(1, &Context->ScissorRect);
            cmdlist->SetDescriptorHeaps(_countof(heaps), heaps);
            cmdlist->OMSetRenderTargets(1, &framebuffer->RTVAllocation.CPUHandle, true, &framebuffer->DSVAllocation.CPUHandle);
            cmdlist->SetGraphicsRootConstantBufferView(ShaderIndicesSimple_Pass, passBuffer.Resource()->GetGPUVirtualAddress());

            for (size_t i = 0; i < MaxItemsPerQueue; i++)
            {
                if (counter == s_DecoupledObjects.size())
                {
                    break;
                }

                auto go = s_DecoupledObjects[counter];

                if (go == nullptr) {
                    continue;
                }

                if (go->Mesh == nullptr) {
                    continue;
                }

                CPUProfileBlock cpuBlock("Simple pass::" + go->Name);
                GPUProfileBlock gpuBlock(myCmdList, "Simple pass::" + go->Name);

                auto tex = go->DecoupledComponent.VirtualTexture;
                tex->Transition(cmdlist, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                auto fm = tex->GetFeedbackMap();

                CreateSRV(std::static_pointer_cast<D3D12Texture>(tex), 0);

                HPerObjectDataSimple objectData;
                objectData.LocalToWorld = go->Transform.LocalToWorldMatrix();
                objectData.FeedbackDims = fm->GetDimensions();
                
                perObjectBuffer.CopyData(i, objectData);

                auto vb = go->Mesh->vertexBuffer->GetView();
                vb.StrideInBytes = sizeof(Vertex);
                auto ib = go->Mesh->indexBuffer->GetView();

                cmdlist->IASetVertexBuffers(0, 1, &vb);
                cmdlist->IASetIndexBuffer(&ib);

                cmdlist->SetGraphicsRootDescriptorTable(ShaderIndicesSimple_Color, tex->SRVAllocation.GPUHandle);
                cmdlist->SetGraphicsRootDescriptorTable(ShaderIndicesSimple_FeedbackMap, fm->UAVAllocation.GPUHandle);
                cmdlist->SetGraphicsRootConstantBufferView(ShaderIndicesSimple_PerObject, perObjectBuffer.GetOffset(i));

                cmdlist->DrawIndexedInstanced(go->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
                ++counter;
            }

            batch.TrackResource(perObjectBuffer.Resource());
            batch.TrackResource(passBuffer.Resource());
            renderTasks.push_back(std::move(
                batch.EndAsync(Context->DeviceResources->CommandQueue)
            ));
        }

        for (auto& task : renderTasks)
        {
            task.wait();
        }
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

            auto shader = Hazel::CreateRef<D3D12Shader>(ShaderPathDecoupled, pipelineStateStream);
            D3D12Renderer::ShaderLibrary->Add(shader);
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


            auto shader = Hazel::CreateRef<D3D12Shader>(ShaderPathSimple, pipelineStateStream);
            D3D12Renderer::ShaderLibrary->Add(shader);
        }
    }

    void DecoupledRenderer::ImplOnFrameEnd()
    {
        for (auto& info : m_DilationQueue)
        {
            TilePool->RemoveTexture(info.Temporary);
        }
        m_DilationQueue.clear();
    }

    void DecoupledRenderer::ImplSubmit(D3D12ResourceBatch& batch, Ref<GameObject> gameObject)
    {
        if (gameObject->DecoupledComponent.UseDecoupledTexture)
        {
            s_DecoupledObjects.push_back(gameObject);
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
            Hazel::D3D12Texture2D::TextureCreationOptions opts;
            opts.Name = name;
            opts.Width = width;
            opts.Height = height;
            opts.MipLevels = mips;
            opts.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            opts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            auto tex = Hazel::D3D12Texture2D::CreateVirtualTexture(batch, opts);
            auto fb = Hazel::D3D12Texture2D::CreateFeedbackMap(batch, tex);
            tex->SetFeedbackMap(fb);
            gameObject->DecoupledComponent.VirtualTexture = std::dynamic_pointer_cast<D3D12VirtualTexture2D>(tex);

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
            CreateSRV(std::static_pointer_cast<D3D12Texture>(tex), 0);
            CreateRTV(std::static_pointer_cast<D3D12Texture>(tex), 0);
        }

    }
}
