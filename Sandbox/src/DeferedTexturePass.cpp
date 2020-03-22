#include "hzpch.h"
#include "DeferedTexturePass.h"
#include "Vertex.h"

#include "Platform/D3D12/D3D12Helpers.h"

// TODO: Maybe get these from an .hlsli file like normal people
static constexpr uint32_t PassCBIndex = 0;
static constexpr uint32_t PerObjectCBIndex = 1;
static constexpr uint32_t SRVIndex = 2;

DeferedTexturePass::DeferedTexturePass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream)
	: m_Context(ctx)
{
	m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/TextureShader.hlsl", pipelineStream);
	Hazel::ShaderLibrary::GlobalLibrary()->Add(m_Shader);
	// We need to create the shader and add get it by name or something here
	m_RTVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		DeferedTexturePass::PassOutputCount
	);

	m_SRVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		DeferedTexturePass::PassInputCount,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);

	m_PassCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<PassData>>(1, true);
	m_PassCB->Resource()->SetName(L"DeferedTexturePass::Scene CB");

	m_PerObjectCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<PerObjectData>>(2, true);
	m_PerObjectCB->Resource()->SetName(L"DeferedTexturePass::Per Object CB");
}

void DeferedTexturePass::Process(Hazel::D3D12Context* ctx, Hazel::GameObject& sceneRoot, Hazel::PerspectiveCamera& camera)
{
	// Create the pass data first

	
	HPassData.ViewProjection = camera.GetViewProjectionMatrix();
	HPassData.CameraPosition = camera.GetPosition();
	// The rest of the light stuff

	m_PassCB->CopyData(0, HPassData);

	// TODO: Maybe do not hardcode this here? Maybe we want to be able to handle children somehow ?
	PerObjectData HPerObjectData;
	HPerObjectData.Glossiness = sceneRoot.Material->Glossines;
	HPerObjectData.ModelMatrix = sceneRoot.Transform.LocalToWorldMatrix();
	auto scale = glm::transpose(glm::inverse(sceneRoot.Transform.ScaleMatrix()));
	auto rot = sceneRoot.Transform.RotationMatrix();
	HPerObjectData.NormalsMatrix = rot * scale;
	m_PerObjectCB->CopyData(0, HPerObjectData);

	// Render
	auto target = m_Outputs[0];
	auto cmdList = ctx->DeviceResources->CommandList;

	D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)target->GetWidth(), (float)target->GetHeight(), 0.0f, 1.0f };
	D3D12_RECT rect = { 0.0f, 0.0f, (float)target->GetWidth(), (float)target->GetHeight() };
	
	cmdList->SetPipelineState(m_Shader->GetPipelineState());
	cmdList->SetGraphicsRootSignature(m_Shader->GetRootSignature());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->RSSetViewports(1, &vp);
	cmdList->RSSetScissorRects(1, &rect);
	cmdList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());

	auto mesh = sceneRoot.Mesh;
	D3D12_VERTEX_BUFFER_VIEW vb = mesh->vertexBuffer->GetView();
	vb.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW ib = mesh->indexBuffer->GetView();

	cmdList->IASetVertexBuffers(0, 1, &vb);
	cmdList->IASetIndexBuffer(&ib);
	cmdList->SetGraphicsRootConstantBufferView(PassCBIndex, m_PassCB->Resource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(PerObjectCBIndex, m_PerObjectCB->Resource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(SRVIndex, m_SRVHeap->GetGPUDescriptorHandleForHeapStart());
	target->Transition(D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtv = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

	cmdList->OMSetRenderTargets(1, &rtv, true, nullptr);

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	m_Context
		->DeviceResources
		->CommandList
		->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	cmdList->DrawIndexedInstanced(mesh->indexBuffer->GetCount(), 1, 0, 0, 0);

	target->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DeferedTexturePass::SetInput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> input)
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

void DeferedTexturePass::SetOutput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> output)
{
	D3D12RenderPass::SetOutput(index, output);

	if (index == 0) {
		m_Context->DeviceResources->Device->CreateRenderTargetView(
			output->GetCommitedResource(),
			nullptr,
			m_RTVHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}
}
