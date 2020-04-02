#include "NormalsDebugPass.h"
#include "Vertex.h"

static constexpr uint32_t PassCBIndex = 0;
static constexpr uint32_t PerObjectCBIndex = 1;

NormalsDebugPass::NormalsDebugPass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream)
	: D3D12RenderPass(ctx)
{
	Hazel::D3D12Shader::OptionalShaderType shaderTypes = Hazel::D3D12Shader::OptionalShaderType::Geometry;
	m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/NormalsDebugShader.hlsl", pipelineStream, shaderTypes);

	Hazel::ShaderLibrary::GlobalLibrary()->Add(m_Shader);

	m_SRVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		NormalsDebugPass::PassInputCount,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);

	m_PassCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<HPassData>>(1, true);
	m_PassCB->Resource()->SetName(L"NormalsDebugPass::Scene CB");

	m_PerObjectCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<HPerObjectData>>(1, true);
	m_PerObjectCB->Resource()->SetName(L"NormalsDebugPass::Per Object CB");

	PassData.NormalColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
	PassData.ReflectColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
	PassData.NormalLength = 1.0f;
}

void NormalsDebugPass::Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera)
{
	PassData.ViewProjection = camera.GetViewProjectionMatrix();
	// Set the others outside
	m_PassCB->CopyData(0, PassData);

	HPerObjectData PerObjectData;
	PerObjectData.LocalToWorld = sceneRoot->Transform.LocalToWorldMatrix();
	auto scale = glm::transpose(glm::inverse(sceneRoot->Transform.ScaleMatrix()));
	auto rot = sceneRoot->Transform.RotationMatrix();
	PerObjectData.NormalMatrix = rot * scale;
	m_PerObjectCB->CopyData(0, PerObjectData);

	auto cmdList = ctx->DeviceResources->CommandList;
	PIXScopedEvent(cmdList.Get(), PIX_COLOR(1, 0, 1), "Normals Pass");

	cmdList->SetPipelineState(m_Shader->GetPipelineState());
	cmdList->SetGraphicsRootSignature(m_Shader->GetRootSignature());
	
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->RSSetViewports(1, &ctx->m_Viewport);
	cmdList->RSSetScissorRects(1, &ctx->m_ScissorRect);
	cmdList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
	cmdList->SetGraphicsRootConstantBufferView(PassCBIndex, m_PassCB->Resource()->GetGPUVirtualAddress());

	// Bind RTV
	auto rtv = ctx->CurrentBackBufferView();
	cmdList->OMSetRenderTargets(1, &rtv, true, &ctx->DepthStencilView());

	auto mesh = sceneRoot->Mesh;
	D3D12_VERTEX_BUFFER_VIEW vb = mesh->vertexBuffer->GetView();
	vb.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW ib = mesh->indexBuffer->GetView();
	cmdList->IASetVertexBuffers(0, 1, &vb);
	cmdList->IASetIndexBuffer(&ib);
	cmdList->SetGraphicsRootConstantBufferView(PerObjectCBIndex,
		m_PerObjectCB->Resource()->GetGPUVirtualAddress());

	cmdList->DrawIndexedInstanced(mesh->indexBuffer->GetCount(), 1, 0, 0, 0);


}

void NormalsDebugPass::BuildConstantsBuffer(Hazel::GameObject* gameObject)
{

}

void NormalsDebugPass::RenderItems(Hazel::TComPtr<ID3D12GraphicsCommandList> cmdList, Hazel::GameObject* goptr)
{

}
