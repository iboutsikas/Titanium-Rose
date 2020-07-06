#include "hzpch.h"

#include "Hazel/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12ForwardRenderer.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/D3D12Shader.h"

#include <future>

#define SHADER_NAME "PbrShader"

void Hazel::D3D12ForwardRenderer::ImplRenderSubmitted(Scene& scene)
{
#if 1
    static std::vector<std::future<void>> renderTasks;

    uint32_t counter = 0;
    auto shader = ShaderLibrary->GetAs<D3D12Shader>(SHADER_NAME);

    HPassData passData;
    passData.ViewProjection = scene.Camera->GetViewProjectionMatrix();

    for (size_t i = 0; i < MaxSupportedLights; i++)
    {
        passData.Lights[i].Position = scene.Lights[i].GameObject->Transform.Position();
        passData.Lights[i].Range = scene.Lights[i].Range;
        passData.Lights[i].Color = scene.Lights[i].GameObject->Material->Color;
        passData.Lights[i].Intensity = scene.Lights[i].Intensity;
    }

    passData.AmbientLight = scene.AmbientLight;
    passData.AmbientIntensity = scene.AmbientIntensity;
    passData.EyePosition = scene.Camera->GetPosition();

    while (counter < s_OpaqueObjects.size())
    {
        D3D12ResourceBatch batch(Context->DeviceResources->Device);
        auto cmdList = batch.Begin();

        D3D12UploadBuffer<HPerObjectData> perObjectBuffer(batch, MaxItemsPerQueue, true);
        D3D12UploadBuffer<HPassData> passBuffer(batch, 1, true);
        passBuffer.CopyData(0, passData);

        cmdList->SetPipelineState(shader->GetPipelineState());
        cmdList->SetGraphicsRootSignature(shader->GetRootSignature());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->RSSetViewports(1, &Context->m_Viewport);
        cmdList->RSSetScissorRects(1, &Context->m_ScissorRect);

        ID3D12DescriptorHeap* const heaps[] = {
            ResourceDescriptorHeap->GetHeap()
        };
        cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

        auto rtv = Context->CurrentBackBufferView();
        cmdList->OMSetRenderTargets(1, &rtv, true, &D3D12Renderer::Context->DepthStencilView());

        cmdList->SetGraphicsRootConstantBufferView(ShaderIndices_Pass, passBuffer.Resource()->GetGPUVirtualAddress());

        for (size_t i = 0; i < MaxItemsPerQueue; i++)
        {
            if (counter == s_OpaqueObjects.size())
            {
                break;
            }

            auto& go = s_OpaqueObjects[counter];

            if (go == nullptr) {
                continue;
            }

            if (go->Mesh == nullptr) {
                continue;
            }

            HPerObjectData objectData;
            objectData.LocalToWorld = go->Transform.LocalToWorldMatrix();
            objectData.WorldToLocal = glm::transpose(go->Transform.WorldToLocalMatrix());
            objectData.HasAlbedo = go->Material->HasAlbedoTexture;
            objectData.HasNormal = go->Material->HasNormalTexture;
            objectData.HasSpecular = go->Material->HasSpecularTexture;
            objectData.Specular = go->Material->Specular;
            objectData.Color = go->Material->Color;

            perObjectBuffer.CopyData(i, objectData);

            auto vb = go->Mesh->vertexBuffer->GetView();
            vb.StrideInBytes = sizeof(Vertex);
            auto ib = go->Mesh->indexBuffer->GetView();

            cmdList->IASetVertexBuffers(0, 1, &vb);
            cmdList->IASetIndexBuffer(&ib);
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Albedo, go->Material->AlbedoTexture->DescriptorAllocation.GPUHandle);
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Normal, go->Material->NormalTexture->DescriptorAllocation.GPUHandle);
            cmdList->SetGraphicsRootDescriptorTable(ShaderIndices_Specular, go->Material->SpecularTexture->DescriptorAllocation.GPUHandle);

            cmdList->SetGraphicsRootConstantBufferView(ShaderIndices_PerObject,
                (perObjectBuffer.Resource()->GetGPUVirtualAddress() + perObjectBuffer.CalculateOffset(i))
            );

            cmdList->DrawIndexedInstanced(go->Mesh->indexBuffer->GetCount(), 1, 0, 0, 0);

            ++counter;
        }
        batch.TrackResource(perObjectBuffer.Resource());
        batch.TrackResource(passBuffer.Resource());
        renderTasks.push_back(batch.End(Context->DeviceResources->CommandQueue.Get()));

        for (auto& task : renderTasks)
        {
            task.wait();
        }

        renderTasks.clear();
    }
#endif
}

void Hazel::D3D12ForwardRenderer::ImplOnInit()
{
    {
        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
        rasterizer.FrontCounterClockwise = TRUE;
        rasterizer.CullMode = D3D12_CULL_MODE_BACK;

        Hazel::D3D12Shader::PipelineStateStream pipelineStateStream;

        pipelineStateStream.InputLayout = { s_InputLayout, s_InputLayoutCount };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        pipelineStateStream.RTVFormats = rtvFormats;
        pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);
        auto shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/" SHADER_NAME ".hlsl", pipelineStateStream);
        D3D12Renderer::ShaderLibrary->Add(shader);
    }
}
