#include "ClearFeedbackPass.h"
#include "Platform/D3D12/D3D12Helpers.h"

static constexpr char* ClearPath = "assets/shaders/ClearUAVShader.hlsl";
static constexpr uint32_t PerObjectCBIndex = 0;
static constexpr uint32_t BuffersIndex = 1;

ClearFeedbackPass::ClearFeedbackPass(Hazel::D3D12Context* ctx)
	: D3D12RenderPass(ctx)
{
	Hazel::D3D12Shader::PipelineStateStream stream;
	if (Hazel::ShaderLibrary::GlobalLibrary()->Exists(ClearPath))
	{
		m_Shader = Hazel::ShaderLibrary::GlobalLibrary()->GetAs<Hazel::D3D12Shader>(ClearPath);
	}
	else
	{
		m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>(ClearPath, stream, Hazel::ShaderType::Compute);
		Hazel::ShaderLibrary::GlobalLibrary()->Add(m_Shader);
	}

	m_SRVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		ClearFeedbackPass::PassInputCount,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);

	m_SRVHeap->SetName(L"ClearFeedbackPass::ResourceHeap");

	m_PerObjectCB = Hazel::CreateRef<Hazel::D3D12UploadBuffer<HPerObjectData>>(PassInputCount, true);
	m_PerObjectCB->Resource()->SetName(L"ClearFeedbackPass::Per Object CB");
}

void ClearFeedbackPass::Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera)
{
	auto cmdList = ctx->DeviceResources->CommandList;

	PIXScopedEvent(cmdList.Get(), PIX_COLOR(1, 0, 1), "Clear Feedback Pass");

	cmdList->SetComputeRootSignature(m_Shader->GetRootSignature());
	cmdList->SetPipelineState(m_Shader->GetPipelineState());

	cmdList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());

	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
		0,
		ctx->GetSRVDescriptorSize()
	);
	cmdList->SetComputeRootDescriptorTable(BuffersIndex, m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

	for (size_t i = 0; i < PassInputCount; i++)
	{
		auto feedback = m_Inputs[i]->GetFeedbackMap();
		ID3D12Resource* resource = nullptr;  
		glm::ivec3 dims = glm::ivec3(1, 1, 1);
		if (feedback != nullptr)
		{
			resource = feedback->GetResource();
			auto desc = resource->GetDesc();
			dims = feedback->GetDimensions();
		}

		auto dispatch_count = Hazel::D3D12::RoundToMultiple(dims.x * dims.y, 64);

		// In here I need to populate the constants buffer
		HPerObjectData od;
		od.ClearValue = m_Inputs[i]->GetMipLevels();
		od.TextureIndex = i;
		m_PerObjectCB->CopyData(i, od);

		cmdList->SetComputeRootConstantBufferView(PerObjectCBIndex,
			m_PerObjectCB->Resource()->GetGPUVirtualAddress() + m_PerObjectCB->CalculateOffset(i));

		
		cmdList->Dispatch(dispatch_count, 1, 1);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(resource));
	}
}

void ClearFeedbackPass::SetInput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> input)
{
	if (index > PassInputCount) {
		HZ_CORE_ASSERT(false, "This pass does not have this input");
	}

	m_Inputs[index] = input;

	auto cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
		index,
		m_Context->GetSRVDescriptorSize()
	);

	auto feedback = input->GetFeedbackMap();
	ID3D12Resource* resource = nullptr;

	D3D12_UNORDERED_ACCESS_VIEW_DESC fbUAVDesc = {};
	fbUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	fbUAVDesc.Format = DXGI_FORMAT_R32_UINT;
	fbUAVDesc.Buffer.NumElements = 1;

	if (feedback != nullptr) {
		resource = feedback->GetResource();
		auto fbDesc = resource->GetDesc();

		fbUAVDesc.Format = fbDesc.Format;
		auto dims = feedback->GetDimensions();
		fbUAVDesc.Buffer.NumElements = dims.x * dims.y;
		fbUAVDesc.Buffer.StructureByteStride = sizeof(uint32_t);
	}

	m_Context->DeviceResources->Device->CreateUnorderedAccessView(
		resource,
		nullptr,
		&fbUAVDesc,
		cpuHandle
	);
}