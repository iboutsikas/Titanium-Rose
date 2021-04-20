#include "hzpch.h"

#include "Hazel/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12ForwardRenderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include "winpixeventruntime/pix3.h"

#include <future>
#include <sstream>

DECLARE_SHADER_NAMED("SurfaceShader-Forward", Surface);

void Hazel::D3D12ForwardRenderer::ImplRenderSubmitted()
{
#if 1
    auto commandList = Context->DeviceResources->MainCommandList;
    if (commandList->IsClosed())
        commandList->Reset(Context->CurrentFrameResource->CommandAllocator);

    auto shader = ShaderLibrary->GetAs<D3D12Shader>(ShaderNameSurface);
    auto envRad = s_CommonData.Scene->Environment.EnvironmentMap;
    auto envIrr = s_CommonData.Scene->Environment.IrradianceMap;
    auto lut = TextureLibrary->GetAs<Texture2D>(std::string("spbrdf"));

    HPassData passData;
    passData.ViewProjection = s_CommonData.Scene->Camera->GetViewProjectionMatrix();
    passData.NumLights = s_CommonData.NumLights;
    passData.EyePosition = s_CommonData.Scene->Camera->GetPosition();

    auto framebuffer = ResolveFrameBuffer();
    
    auto cmdList = commandList->Get();
    ScopedTimer passTimer("Forward Pass", commandList);

    D3D12UploadBuffer<HPerObjectData> perObjectBuffer(Context->DeviceResources->Device, s_OpaqueObjects.size(), true);
    D3D12UploadBuffer<HPassData> passBuffer(Context->DeviceResources->Device, 1, true);

    passBuffer.CopyData(0, passData);

    cmdList->SetPipelineState(shader->GetPipelineState());
    cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->RSSetViewports(1, &Context->Viewport);
    cmdList->RSSetScissorRects(1, &Context->ScissorRect);

    ID3D12DescriptorHeap* const heaps[] = {
        s_ResourceDescriptorHeap->GetHeap()
    };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);


    cmdList->OMSetRenderTargets(1, &framebuffer->RTVAllocation.CPUHandle, true, &framebuffer->DSVAllocation.CPUHandle);

    cmdList->SetGraphicsRootConstantBufferView(ShaderIndices_Pass, passBuffer.ResourceRaw()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Lights, s_LightsBufferAllocation.GPUHandle);
    cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_EnvRadiance, envRad->SRVAllocation.GPUHandle);
    cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_EnvIrradiance, envIrr->SRVAllocation.GPUHandle);
    cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_BRDFLUT, lut->SRVAllocation.GPUHandle);

    for (size_t i = 0; i < s_OpaqueObjects.size(); i++)
    {
        auto go = s_OpaqueObjects[i];

        if (go == nullptr) {
            continue;
        }

        if (go->Mesh == nullptr) {
            continue;
        }

        ScopedTimer objectTimer(go->Name, commandList);

        HPerObjectData objectData;
        objectData.LocalToWorld = go->Transform.LocalToWorldMatrix();
        objectData.WorldToLocal = glm::transpose(go->Transform.WorldToLocalMatrix());
        objectData.MaterialColor = go->Material->Color;
        objectData.HasAlbedo = go->Material->HasAlbedoTexture;
        objectData.EmissiveColor = go->Material->EmissiveColor;
        objectData.HasNormal = go->Material->HasNormalTexture;
        objectData.HasMetallic = go->Material->HasMatallicTexture;
        objectData.Metallic = go->Material->Metallic;
        objectData.HasRoughness = go->Material->HasRoughnessTexture;
        objectData.Roughness = go->Material->Roughness;
        perObjectBuffer.CopyData(i, objectData);

        auto vb = go->Mesh->vertexBuffer->GetView();
        vb.StrideInBytes = sizeof(Vertex);
        auto ib = go->Mesh->indexBuffer->GetView();

        cmdList->IASetVertexBuffers(0, 1, &vb);
        cmdList->IASetIndexBuffer(&ib);
        if (go->Material->HasAlbedoTexture) {
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Albedo, go->Material->AlbedoTexture->SRVAllocation.GPUHandle);
        }

        if (go->Material->HasNormalTexture) {
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Normal, go->Material->NormalTexture->SRVAllocation.GPUHandle);
        }

        if (go->Material->HasRoughnessTexture) {
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Roughness, go->Material->RoughnessTexture->SRVAllocation.GPUHandle);
        }

        if (go->Material->HasMatallicTexture) {
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Metalness, go->Material->MetallicTexture->SRVAllocation.GPUHandle);
        }

        cmdList->SetGraphicsRootConstantBufferView(ShaderIndices_PerObject,
            (perObjectBuffer.ResourceRaw()->GetGPUVirtualAddress() + perObjectBuffer.CalculateOffset(i))
        );

        cmdList->DrawIndexedInstanced(go->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
    }
    commandList->Track(perObjectBuffer.Resource());
    commandList->Track(passBuffer.Resource());
#endif
}

void Hazel::D3D12ForwardRenderer::ImplOnInit()
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
        auto shader = Hazel::CreateRef<Hazel::D3D12Shader>(std::string(ShaderPathSurface), pipelineStateStream);
        D3D12Renderer::ShaderLibrary->Add(shader);
    }
}

void Hazel::D3D12ForwardRenderer::ImplSubmit(Ref<GameObject> gameObject)
{
    if (gameObject->Material->IsTransparent)
    {
        s_TransparentObjects.push_back(gameObject);
    }
    else if (!gameObject->DecoupledComponent.UseDecoupledTexture)
    {
        s_OpaqueObjects.push_back(gameObject);
    }
}
