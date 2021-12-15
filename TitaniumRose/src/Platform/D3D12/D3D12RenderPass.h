#pragma once

#include "TitaniumRose/ComponentSystem/GameObject.h"
#include "TitaniumRose/Core/Core.h"
#include "TitaniumRose/Renderer/PerspectiveCamera.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "d3d12.h"

#include "WinPixEventRuntime/pix3.h"

namespace Roses {

	template <uint32_t TNumInputs, uint32_t TNumOutputs>
	class D3D12RenderPass
	{
	public:
		static constexpr uint32_t PassOutputCount = TNumOutputs;
		static constexpr uint32_t PassInputCount = TNumInputs;

		D3D12RenderPass(Roses::D3D12Context* ctx)
		: m_Context(ctx)
		{}

		virtual ~D3D12RenderPass() = default;

		virtual void Process(D3D12Context* ctx, Roses::HGameObject* sceneRoot, Roses::PerspectiveCamera& camera) = 0;
	
		virtual void SetInput(uint32_t index, Roses::Ref<Texture2D> input) 
		{
			if (index < PassInputCount) {
				m_Inputs[index] = input;
			}
			else {
				HZ_CORE_ASSERT(false, "This pass does not have this input");
			}

			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
				m_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
				index,
				m_Context->GetSRVDescriptorSize()
			);
			//D3D12_SHADER_RESOURCE_VIEW_DESC
			// If SetInput did not throw we are in a valid range;
			m_Context->DeviceResources->Device->CreateShaderResourceView(
				input->GetResource(),
				nullptr,
				srvHandle
			);
		}

		virtual Roses::Ref<Texture2D> GetInput(uint32_t index) const 
		{
			if (index < PassInputCount) {
				return m_Inputs[index];
			}
			HZ_CORE_ASSERT(false, "This pass does not have this input");
			return nullptr;
		}

		virtual void SetOutput(uint32_t index, Roses::Ref<Texture2D> output) 
		{
			if (index < PassOutputCount) {
				m_Outputs[index] = output;
			}
			else {
				HZ_CORE_ASSERT(false, "This pass does not have this output");
			}
		};

		virtual Roses::Ref<Texture2D> GetOutput(uint32_t index) const
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

		Roses::Ref<Roses::D3D12Shader> m_Shader;
		Roses::Ref<Roses::Texture2D> m_Inputs[TNumInputs];
		Roses::Ref<Roses::Texture2D> m_Outputs[TNumOutputs];
		Roses::TComPtr<ID3D12DescriptorHeap> m_SRVHeap;
		Roses::D3D12Context* m_Context;
	};
}
