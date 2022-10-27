#include "trpch.h"

#include "TitaniumRose/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12ForwardRenderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "winpixeventruntime/pix3.h"

#include <future>
#include <sstream>

DECLARE_SHADER_NAMED("SurfaceShader-Forward", Surface);

void Roses::D3D12ForwardRenderer::ImplRenderSubmitted(GraphicsContext& gfxContext)
{
#if 1
   
    auto shader = g_ShaderLibrary->GetAs<D3D12Shader>(ShaderNameSurface);
    auto envRad = s_CommonData.Scene->Environment.EnvironmentMap;
    auto envIrr = s_CommonData.Scene->Environment.IrradianceMap;
    auto lut = g_TextureLibrary->GetAs<Texture2D>(std::string("spbrdf"));

    HPassData passData;
    passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();
    passData.NumLights = s_CommonData.NumLights;
    passData.EyePosition = s_CommonData.Scene->Camera->GetPosition();

    auto framebuffer = ResolveFrameBuffer();
    
    ScopedTimer passTimer("Forward Pass", gfxContext);

    gfxContext.GetCommandList()->SetPipelineState(shader->GetPipelineState());
    gfxContext.GetCommandList()->SetGraphicsRootSignature(shader->GetRootSignature());
    gfxContext.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gfxContext.GetCommandList()->RSSetViewports(1, &Context->Viewport);
    gfxContext.GetCommandList()->RSSetScissorRects(1, &Context->ScissorRect);
    gfxContext.GetCommandList()->OMSetRenderTargets(1, &framebuffer->RTVAllocation.CPUHandle, true, &framebuffer->DSVAllocation.CPUHandle);
    gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Lights, s_LightsBufferAllocation.GPUHandle);
    gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_EnvRadiance, envRad->SRVAllocation.GPUHandle);
    gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_EnvIrradiance, envIrr->SRVAllocation.GPUHandle);
    gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_BRDFLUT, lut->SRVAllocation.GPUHandle);
    gfxContext.SetDynamicContantBufferView(ShaderIndices_Pass, sizeof(passData), &passData);

    for (size_t i = 0; i < s_ForwardOpaqueObjects.size(); i++)
    {
        auto go = s_ForwardOpaqueObjects[i];

        if (go == nullptr) {
            continue;
        }

        if (go->Mesh == nullptr) {
            continue;
        }

        std::vector<uint32_t> lights;

        for (uint32_t i = 0; i < s_CommonData.Scene->Lights.size(); i++)
        {
#if 0
            lights.push_back(i);
#else
            auto l = s_CommonData.Scene->Lights[i];

            auto v = l->gameObject->Transform.Position() - go->Transform.Position();

            auto d = glm::length(v);

            if (d <= l->Range * 2 || go->Material->IncludeAllLights) {
                lights.push_back(i);
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
        


        ScopedTimer objectTimer(go->Name, gfxContext);

        HPerObjectData objectData;
        objectData.LocalToWorld = go->Transform.LocalToWorldMatrix();
        objectData.WorldToLocal = glm::transpose(go->Transform.WorldToLocalMatrix());
        objectData.MaterialColor = go->Material->Color;
        objectData.HasAlbedo = go->Material->HasAlbedoTexture;
        objectData.EmissiveColor = go->Material->EmissiveColor;
        objectData.HasNormal = go->Material->HasNormalTexture;
        objectData.HasMetallic = go->Material->HasMetallicTexture;
        objectData.Metallic = go->Material->Metallic;
        objectData.HasRoughness = go->Material->HasRoughnessTexture;
        objectData.Roughness = go->Material->Roughness;
        objectData.NumObjectLights = lights.size();

        auto vb = go->Mesh->vertexBuffer->GetView();
        vb.StrideInBytes = sizeof(Vertex);
        auto ib = go->Mesh->indexBuffer->GetView();

        gfxContext.GetCommandList()->IASetVertexBuffers(0, 1, &vb);
        gfxContext.GetCommandList()->IASetIndexBuffer(&ib);

        if (go->Material->HasAlbedoTexture) {
            gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Albedo, go->Material->AlbedoTexture->SRVAllocation.GPUHandle);
        }

        if (go->Material->HasNormalTexture) {
            gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Normal, go->Material->NormalTexture->SRVAllocation.GPUHandle);
        }

        if (go->Material->HasRoughnessTexture) {
            gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Roughness, go->Material->RoughnessTexture->SRVAllocation.GPUHandle);
        }

        if (go->Material->HasMetallicTexture) {
            gfxContext.GetCommandList()->SetGraphicsRootDescriptorTable(ShaderIndices_Metalness, go->Material->MetallicTexture->SRVAllocation.GPUHandle);
        }

        gfxContext.SetDynamicContantBufferView(ShaderIndices_PerObject, sizeof(objectData), &objectData);

        gfxContext.GetCommandList()->DrawIndexedInstanced(go->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
    }
#endif
}

void Roses::D3D12ForwardRenderer::ImplOnInit()
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
        pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        pipelineStateStream.RTVFormats = rtvFormats;
        pipelineStateStream.RasterizerState = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);
        auto shader = Roses::CreateRef<Roses::D3D12Shader>(std::string(ShaderPathSurface), pipelineStateStream);
        D3D12Renderer::g_ShaderLibrary->Add(shader);
    }
}

void Roses::D3D12ForwardRenderer::ImplSubmit(Ref<HGameObject> gameObject)
{
    if (gameObject->Material->IsTransparent)
    {
        s_ForwardTransparentObjects.push_back(gameObject);
    }
    else if (!gameObject->DecoupledComponent.UseDecoupledTexture)
    {
        s_ForwardOpaqueObjects.push_back(gameObject);
    }
}
