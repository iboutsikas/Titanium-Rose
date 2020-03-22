#include "BaseColorPass.h"

#include "Vertex.h"
#include "glm/gtc/type_ptr.hpp"

static int cbOffset = 0;
static constexpr uint32_t PassCBIndex = 0;
static constexpr uint32_t PerObjectCBIndex = 1;
static constexpr uint32_t SRVIndex = 2;

BaseColorPass::BaseColorPass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream)
	: m_Context(ctx)
{
	m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/BasicShader.hlsl", pipelineStream);
	Hazel::ShaderLibrary::GlobalLibrary()->Add(m_Shader);
	
	m_SRVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		BaseColorPass::PassInputCount,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);

	m_PassCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<PassData>>(1, true);
	m_PassCB->Resource()->SetName(L"BaseColorPass::Scene CB");

	m_PerObjectCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<PerObjectData>>(2, true);
	m_PerObjectCB->Resource()->SetName(L"BaseColorPass::Per Object CB");
}

void BaseColorPass::Process(Hazel::D3D12Context* ctx, Hazel::GameObject& sceneRoot, Hazel::PerspectiveCamera& camera)
{
	HPassData.ViewProjection = camera.GetViewProjectionMatrix();
	m_PassCB->CopyData(0, HPassData);

	cbOffset = 0;
	BuildConstantsBuffer(&sceneRoot);

	auto cmdList = m_Context->DeviceResources->CommandList;

	cmdList->SetPipelineState(m_Shader->GetPipelineState());
	cmdList->SetGraphicsRootSignature(m_Shader->GetRootSignature());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->RSSetViewports(1, &ctx->m_Viewport);
	cmdList->RSSetScissorRects(1, &ctx->m_ScissorRect);
	cmdList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
	cmdList->SetGraphicsRootConstantBufferView(PassCBIndex, m_PassCB->Resource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(SRVIndex, m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

	// Bind RTV
	auto rtv = ctx->CurrentBackBufferView();
	cmdList->OMSetRenderTargets(1, &rtv, true, &ctx->DepthStencilView());
	// Clear RTV
	cmdList->ClearRenderTargetView(rtv, glm::value_ptr(ClearColor), 0, nullptr);
	cmdList->ClearDepthStencilView(
		ctx->DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr
	);

	RenderItems(cmdList, &sceneRoot);
}

void BaseColorPass::SetInput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> input)
{
	D3D12RenderPass::SetInput(index, input);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
		m_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
		index,
		m_Context->GetSRVDescriptorSize()
	);

	// If SetInput did not throw we are in a valid range;
	m_Context->DeviceResources->Device->CreateShaderResourceView(
		input->GetCommitedResource(),
		nullptr,
		srvHandle
	);
}

void BaseColorPass::BuildConstantsBuffer(Hazel::GameObject* goptr)
{
	if (goptr == nullptr)
		return;
	
	PerObjectData od;
	od.LocalToWorld = goptr->Transform.LocalToWorldMatrix();
	od.TextureIndex = goptr->Material->TextureId;
	goptr->Material->cbIndex = cbOffset++;
	m_PerObjectCB->CopyData(goptr->Material->cbIndex, od);

	for (auto& child : goptr->children)
	{
		BuildConstantsBuffer(child);
	}
}

void BaseColorPass::RenderItems(Hazel::TComPtr<ID3D12GraphicsCommandList> cmdList, Hazel::GameObject* goptr)
{
	if (goptr == nullptr)
		return;

	auto mesh = goptr->Mesh;
	D3D12_VERTEX_BUFFER_VIEW vb = mesh->vertexBuffer->GetView();
	vb.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW ib = mesh->indexBuffer->GetView();
	cmdList->IASetVertexBuffers(0, 1, &vb);
	cmdList->IASetIndexBuffer(&ib);
	cmdList->SetGraphicsRootConstantBufferView(PerObjectCBIndex, 
		m_PerObjectCB->Resource()->GetGPUVirtualAddress() + m_PerObjectCB->CalculateOffset(goptr->Material->cbIndex));

	cmdList->DrawIndexedInstanced(mesh->indexBuffer->GetCount(), 1, 0, 0, 0);

	for (auto& child : goptr->children) {
		RenderItems(cmdList, child);
	}
}
