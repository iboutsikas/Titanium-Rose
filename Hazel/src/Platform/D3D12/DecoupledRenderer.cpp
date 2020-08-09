#include "hzpch.h"
#include "Platform/D3D12/DecoupledRenderer.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "winpixeventruntime/pix3.h"

namespace Hazel
{
    DECLARE_SHADER_NAMED("SurfaceShader-Decoupled", Decoupled);
    DECLARE_SHADER_NAMED("SurfaceShader-Simple", Simple);
    // 
    void DecoupledRenderer::ImplRenderVirtualTextures()
    {
        HeapValidationMark rtvMark(s_RenderTargetDescriptorHeap);
        HeapValidationMark srvMark(s_ResourceDescriptorHeap);

        auto shader = GetShader();
        auto envRad = s_CommonData.Scene->Environment.EnvironmentMap;
        auto envIrr = s_CommonData.Scene->Environment.IrradianceMap;
        auto lut = TextureLibrary->GetAs<D3D12Texture2D>(std::string("spbrdf"));

        HPassData passData;
        passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();
        passData.NumLights = s_CommonData.NumLights;
        passData.EyePosition = s_CommonData.Scene->Camera->GetPosition();

        std::vector<std::future<void>> renderTasks(std::ceil<size_t>(s_DecoupledObjects.size() / MaxItemsPerQueue));
        
        uint32_t counter = 0;
        while (counter < s_DecoupledObjects.size())
        {
            D3D12ResourceBatch batch(Context->DeviceResources->Device);
            auto cmdlist = batch.Begin();
            
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

                auto& tex = obj->DecoupledComponent.VirtualTexture;
                PIXBeginEvent(cmdlist.Get(), PIX_COLOR(200, 0, 0), "Virtual Pass: %s", tex->GetIdentifier().c_str());

                tex->Transition(cmdlist.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
                auto mips = tex->GetMipsUsed();

                CreateRTV(std::static_pointer_cast<D3D12Texture>(tex), mips.FinestMip);

                D3D12_VIEWPORT vp = { 0, 0, tex->GetWidth() >> mips.FinestMip, tex->GetHeight() >> mips.FinestMip, 0, 1 };
                D3D12_RECT rect = { 0, 0, tex->GetWidth() >> mips.FinestMip, tex->GetHeight() >> mips.FinestMip };

                cmdlist->RSSetViewports(1, &vp);
                cmdlist->RSSetScissorRects(1, &rect);
                cmdlist->OMSetRenderTargets(1, &tex->RTVAllocation.CPUHandle, true, nullptr);
                static const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
                cmdlist->ClearRenderTargetView(tex->RTVAllocation.CPUHandle, clearColor, 0, nullptr);

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
                tex->Transition(cmdlist.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                counter++;
                PIXEndEvent(cmdlist.Get());
            }
            batch.TrackResource(perObjectBuffer.Resource());
            batch.TrackResource(passBuffer.Resource());
            renderTasks.push_back(batch.End(Context->DeviceResources->CommandQueue.Get()));
        }

        for (auto& task : renderTasks)
        {
            task.wait();
        }
        renderTasks.clear();
    }

    Ref<D3D12Shader> DecoupledRenderer::GetShader()
    {
        return ShaderLibrary->GetAs<D3D12Shader>(ShaderNameDecoupled);
    }

    void DecoupledRenderer::ImplRenderSubmitted()
    {
        //HeapValidationMark rtvMark(s_RenderTargetDescriptorHeap);
        //HeapValidationMark srvMark(s_ResourceDescriptorHeap);

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


        while (counter < s_DecoupledObjects.size())
        {
            D3D12ResourceBatch batch(Context->DeviceResources->Device);
            auto cmdlist = batch.Begin();

            PIXBeginEvent(cmdlist.Get(), PIX_COLOR(255, 0, 255), "Decoupled Geometry Pass: %d", counter / MaxItemsPerQueue);

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

                auto tex = go->DecoupledComponent.VirtualTexture;
                tex->Transition(cmdlist.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                auto fm = tex->GetFeedbackMap();

                //CreateSRV(std::static_pointer_cast<D3D12Texture>(tex), 0);

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
            PIXEndEvent(cmdlist.Get());
            renderTasks.emplace_back(batch.End(Context->DeviceResources->CommandQueue.Get()));
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
            rasterizer.CullMode = D3D12_CULL_MODE_BACK;

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
                width = height = 1024;
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
