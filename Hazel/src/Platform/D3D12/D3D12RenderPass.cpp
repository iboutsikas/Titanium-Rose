#include "hzpch.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12RenderPass.h"
#include "Platform/D3D12/d3dx12.h"

namespace Hazel {

	D3D12RenderPass::D3D12RenderPass(uint32_t frameLatency)
		: m_FrameLatency(frameLatency), 
		  m_FrameCount(0)
	{
	}

	D3D12RenderPass::~D3D12RenderPass()
	{
	}

	void D3D12RenderPass::Update(Timestep ts)
	{
		m_FrameCount++;
		
		if (m_FrameCount >= m_FrameLatency) {
			m_FrameCount = 0;
			
			if (m_PrivateShader != nullptr) {
				m_PublicShader = m_PrivateShader;
				m_PrivateShader = nullptr;
			}

			if (m_PrivateRootSignature != nullptr) {
				m_PublicRootSignature = m_PrivateRootSignature;
				m_PrivateRootSignature = nullptr;
			}

			if (m_PrivatePipelineState != nullptr) {
				m_PublicPipelineState = m_PrivatePipelineState;
				m_PrivatePipelineState = nullptr;
			}
		}
	}
}

