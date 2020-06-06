#include "BaseColorPass.h"

#include "Vertex.h"
#include "glm/gtc/type_ptr.hpp"

static int cbOffset = 0;
static constexpr uint32_t PassCBIndex = 0;
static constexpr uint32_t PerObjectCBIndex = 1;
static constexpr uint32_t SRVIndex = 2;
static constexpr char* ShaderPath = "assets/shaders/BasicShader.hlsl";

BaseColorPass::BaseColorPass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream)
	: D3D12RenderPass(ctx)
{
	if (Hazel::ShaderLibrary::GlobalLibrary()->Exists(ShaderPath))
	{
		m_Shader = Hazel::ShaderLibrary::GlobalLibrary()->GetAs<Hazel::D3D12Shader>(ShaderPath);
	}
	else
	{
		m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>(ShaderPath, pipelineStream);
		Hazel::ShaderLibrary::GlobalLibrary()->Add(m_Shader);
	}
	
	// The +1 is for the feedback resource
	m_SRVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		BaseColorPass::PassInputCount * 2,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);

	m_PassCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<HPassData>>(1, true);
	m_PassCB->Resource()->SetName(L"BaseColorPass::Scene CB");

	m_PerObjectCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<HPerObjectData>>(3, true);
	m_PerObjectCB->Resource()->SetName(L"BaseColorPass::Per Object CB");
}

void BaseColorPass::Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera)
{
	HZ_PROFILE_FUNCTION();
	PassData.ViewProjection = camera.GetViewProjectionMatrix();
	m_PassCB->CopyData(0, PassData);

	cbOffset = 0;
	BuildConstantsBuffer(sceneRoot);

	auto cmdList = m_Context->DeviceResources->CommandList;
	PIXScopedEvent(cmdList.Get(), PIX_COLOR(1, 0, 1), "Base Color Pass");

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

	RenderItems(cmdList, sceneRoot);
}

void BaseColorPass::BuildConstantsBuffer(Hazel::GameObject* goptr)
{
	if (goptr == nullptr)
		return;
	
	// TODO: We need a better system. Probably something like ECS
	if (goptr->Mesh != nullptr) {
		HPerObjectData od;
		od.LocalToWorld = goptr->Transform.LocalToWorldMatrix();
		od.MaterialColor = goptr->Material->Color;
		od.TextureDims = glm::vec4(
			goptr->Material->DiffuseTexture->GetWidth(),
			goptr->Material->DiffuseTexture->GetHeight(),
			1.0f / goptr->Material->DiffuseTexture->GetWidth(),
			1.0f / goptr->Material->DiffuseTexture->GetHeight()
		);
		
		auto dims = goptr->Material->DiffuseTexture->GetFeedbackMap()->GetDimensions();
		od.FeedbackDims.x = dims.x;
		od.FeedbackDims.y = dims.y;


		for (size_t i = 0; i < PassInputCount; i++)
		{
			if (m_Inputs[i] == goptr->Material->DiffuseTexture) {
				od.TextureIndex = i;
				break;
			}
		}
		goptr->Material->cbIndex = cbOffset++;
		m_PerObjectCB->CopyData(goptr->Material->cbIndex, od);
	}	

	for (auto& child : goptr->children)
	{
		BuildConstantsBuffer(child.get());
	}
}

void BaseColorPass::RenderItems(Hazel::TComPtr<ID3D12GraphicsCommandList> cmdList, Hazel::GameObject* goptr)
{
	if (goptr == nullptr)
		return;

	// TODO: We need a better system. Probably something like ECS
	if (goptr->Mesh != nullptr) {
		auto mesh = goptr->Mesh;
		D3D12_VERTEX_BUFFER_VIEW vb = mesh->vertexBuffer->GetView();
		vb.StrideInBytes = sizeof(Vertex);
		D3D12_INDEX_BUFFER_VIEW ib = mesh->indexBuffer->GetView();

		auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
		auto gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

		// Bind the Feedback View
		HPerObjectData* podata = m_PerObjectCB->ElementAt(goptr->Material->cbIndex);
		cpuHandle.Offset(BaseColorPass::PassInputCount + podata->TextureIndex, m_Context->GetSRVDescriptorSize());
		gpuHandle.Offset(BaseColorPass::PassInputCount + podata->TextureIndex, m_Context->GetSRVDescriptorSize());

		auto feedback = goptr->Material->DiffuseTexture->GetFeedbackMap();
		auto fbResource = feedback->GetResource();
		D3D12_UNORDERED_ACCESS_VIEW_DESC fbUAVDesc = {};
		fbUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		fbUAVDesc.Format = DXGI_FORMAT_R32_UINT;
		fbUAVDesc.Buffer.NumElements = 1;
#if 1
		if (fbResource != nullptr) {
			auto fbDesc = fbResource->GetDesc();

			fbUAVDesc.Format = fbDesc.Format;
			auto dims = feedback->GetDimensions();
			fbUAVDesc.Buffer.NumElements = dims.x * dims.y;
			fbUAVDesc.Buffer.StructureByteStride = sizeof(uint32_t);
		}
#endif	
		m_Context->DeviceResources->Device->CreateUnorderedAccessView(
			fbResource,
			nullptr,
			&fbUAVDesc,
			cpuHandle
		);
		
		cmdList->IASetVertexBuffers(0, 1, &vb);
		cmdList->IASetIndexBuffer(&ib);
		cmdList->SetGraphicsRootConstantBufferView(PerObjectCBIndex,
			m_PerObjectCB->Resource()->GetGPUVirtualAddress() + m_PerObjectCB->CalculateOffset(goptr->Material->cbIndex));

		cmdList->DrawIndexedInstanced(mesh->indexBuffer->GetCount(), 1, 0, 0, 0);
	}
	
	for (auto& child : goptr->children) {
		RenderItems(cmdList, child.get());
	}
}
