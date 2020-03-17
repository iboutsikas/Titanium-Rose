#pragma once
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Core/Timestep.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "d3d12.h"

namespace Hazel {

	class D3D12RenderPass
	{
	public:
		D3D12RenderPass(uint32_t frameLatency);
		virtual ~D3D12RenderPass();

		virtual void Update(Timestep ts);
		virtual void Process(D3D12Context* ctx) = 0;
		virtual void Recompile(D3D12Context* ctx) = 0;

		void SetLayout(D3D12_INPUT_ELEMENT_DESC*& layout, uint32_t count);

	protected:

		uint32_t m_FrameLatency;
		uint32_t m_FrameCount;

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
		
		Hazel::Ref<Hazel::D3D12Shader> m_PublicShader;
		Hazel::TComPtr<ID3D12RootSignature> m_PublicRootSignature;
		Hazel::TComPtr<ID3D12PipelineState> m_PublicPipelineState;

		Hazel::Ref<Hazel::D3D12Shader> m_PrivateShader;
		Hazel::TComPtr<ID3D12RootSignature> m_PrivateRootSignature;
		Hazel::TComPtr<ID3D12PipelineState> m_PrivatePipelineState;
	};
}

