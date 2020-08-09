#pragma once

#include "Platform/D3D12/D3D12RenderPass.h"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

class BaseColorPass : public Hazel::D3D12RenderPass<3, 1>
{
public:
	struct alignas(16) HPassData {
		glm::mat4 ViewProjection;
	};

	BaseColorPass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream);
	
	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera) override;

	HPassData PassData;
	glm::vec4 ClearColor;
private:
	void BuildConstantsBuffer(Hazel::GameObject* goptr);
	void RenderItems(Hazel::TComPtr<ID3D12GraphicsCommandList> cmdList, Hazel::GameObject* goptr);


	struct alignas(16) HPerObjectData {
		glm::mat4 LocalToWorld;
		glm::vec4 MaterialColor;
		glm::vec4 TextureDims;
		glm::ivec2 FeedbackDims;
		uint32_t TextureIndex;
	};

	Hazel::Ref<Hazel::D3D12UploadBuffer<HPassData>> m_PassCB;
	Hazel::Ref<Hazel::D3D12UploadBuffer<HPerObjectData>> m_PerObjectCB;
};

