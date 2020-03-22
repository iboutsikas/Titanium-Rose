#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Core/Core.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/PerspectiveCamera.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "d3d12.h"

namespace Hazel {

	template <uint32_t TNumInputs, uint32_t TNumOutputs>
	class D3D12RenderPass
	{
	public:
		static constexpr uint32_t PassOutputCount = TNumOutputs;
		static constexpr uint32_t PassInputCount = TNumInputs;

		D3D12RenderPass() = default;
		virtual ~D3D12RenderPass() = default;

		virtual void Process(D3D12Context* ctx, Hazel::GameObject& sceneRoot, Hazel::PerspectiveCamera& camera) = 0;
	
		virtual void SetInput(uint32_t index, Hazel::Ref<D3D12Texture2D> input) 
		{
			if (index < TNumInputs) {
				m_Inputs[index] = input;
			}
			else {
				HZ_CORE_ASSERT(false, "This pass does not have this input");
			}
		}

		virtual Hazel::Ref<D3D12Texture2D> GetInput(uint32_t index) const 
		{
			if (index < TNumInputs) {
				return m_Inputs[index];
			}
			HZ_CORE_ASSERT(false, "This pass does not have this input");
			return nullptr;
		}

		virtual void SetOutput(uint32_t index, Hazel::Ref<D3D12Texture2D> output) 
		{
			if (index < TNumInputs) {
				m_Outputs[index] = output;
			}
			else {
				HZ_CORE_ASSERT(false, "This pass does not have this output");
			}
		};

		virtual Hazel::Ref<D3D12Texture2D> GetOutput(uint32_t index) const
		{
			if (index < PassOutputCount) {
				return m_Outputs[index];
			}
			HZ_CORE_ASSERT(false, "This pass does not have this output");
			return nullptr;
		}

		//struct PipelineStateStream
		//{
		//	CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		//	CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		//	CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		//	CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		//	CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		//	CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		//	CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		//} m_PipelineStateStream;

	protected:

		Hazel::Ref<Hazel::D3D12Shader> m_Shader;
		Hazel::Ref<Hazel::D3D12Texture2D> m_Inputs[TNumInputs];
		Hazel::Ref<Hazel::D3D12Texture2D> m_Outputs[TNumOutputs];
		Hazel::TComPtr<ID3D12DescriptorHeap> m_SRVHeap;
	};
}
