#include "hzpch.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12RenderPass.h"
#include "Platform/D3D12/d3dx12.h"

namespace Hazel {
	D3D12RenderPass::D3D12RenderPass()
	{
		m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());

		m_PipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_PipelineStateStream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	
	D3D12RenderPass::~D3D12RenderPass()
	{
	}

	void D3D12RenderPass::EnableGBuffer()
	{
	}

	void D3D12RenderPass::AddOutput(uint32_t width, uint32_t height)
	{
	}

	void D3D12RenderPass::Finalize()
	{
		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &m_PipelineStateStream
		};

		Hazel::D3D12::ThrowIfFailed(
			m_Context->DeviceResources->Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState))
		);
	}

	void D3D12RenderPass::SetShader(Ref<Shader> shader)
	{
		m_Shader = std::dynamic_pointer_cast<D3D12Shader>(shader);
		m_PipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(m_Shader->GetVertexBlob().Get());
		m_PipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(m_Shader->GetFragmentBlob().Get());
	}
	
	void D3D12RenderPass::SetLayout(D3D12_INPUT_ELEMENT_DESC*& layout, uint32_t count)
	{
		m_PipelineStateStream.InputLayout = { layout, count };
	}
	
	void D3D12RenderPass::SetRootSignature(ID3D12RootSignature* rootSignature)
	{
		m_PipelineStateStream.pRootSignature = rootSignature;
	}
}

