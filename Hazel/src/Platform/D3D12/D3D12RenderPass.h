#pragma once
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/RenderPass.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "d3d12.h"

namespace Hazel {

	class D3D12RenderPass : public RenderPass
	{
	public:
		D3D12RenderPass();
		virtual ~D3D12RenderPass();

		// Inherited via RenderPass
		virtual void EnableGBuffer() override;
		virtual void AddOutput(uint32_t width, uint32_t height) override;
		virtual void Finalize() override;
		virtual void SetShader(Ref<Shader> shader) override;

		void SetLayout(D3D12_INPUT_ELEMENT_DESC*& layout, uint32_t count);
		void SetRootSignature(ID3D12RootSignature* rootSignature);

	private:

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		} m_PipelineStateStream;
		
		struct D3D12PassOutput {
			uint32_t width;
			uint32_t height;
			DXGI_FORMAT format;
			RenderPassOutputType type;
			TComPtr<ID3D12Resource> resource;
		};

		Hazel::D3D12Context* m_Context;
		Hazel::Ref<Hazel::D3D12Shader> m_Shader;
		Hazel::TComPtr<ID3D12RootSignature> m_RootSignature;
		Hazel::TComPtr<ID3D12PipelineState> m_PipelineState;
	};
}

