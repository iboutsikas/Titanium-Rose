#pragma once

#include "Platform/D3D12/D3D12RenderPass.h"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

class BaseColorPass : public Hazel::D3D12RenderPass<2, 1>
{
public:
	struct alignas(16) PassData {
		glm::mat4 ViewProjection;
	};

	BaseColorPass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream);
	
	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject& sceneRoot, Hazel::PerspectiveCamera& camera) override;
	virtual void SetInput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> input) override;

	PassData HPassData;
	glm::vec4 ClearColor;
private:
	void BuildConstantsBuffer(Hazel::GameObject* goptr);
	void RenderItems(Hazel::TComPtr<ID3D12GraphicsCommandList> cmdList, Hazel::GameObject* goptr);


	struct alignas(16) PerObjectData {
		glm::mat4 LocalToWorld;
		uint32_t TextureIndex;
	};

	Hazel::Ref<Hazel::D3D12UploadBuffer<PassData>> m_PassCB;
	Hazel::Ref<Hazel::D3D12UploadBuffer<PerObjectData>> m_PerObjectCB;
	Hazel::D3D12Context* m_Context;
};

