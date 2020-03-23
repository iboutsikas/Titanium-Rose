#pragma once

#include "Platform/D3D12/D3D12RenderPass.h"

class NormalsDebugPass: public Hazel::D3D12RenderPass<1, 1>
{
public:
	struct alignas(16) HPassData {
		glm::mat4 ViewProjection;
		glm::vec4 NormalColor;
		glm::vec4 ReflectColor;
		glm::vec3 LightPosition;
		float  NormalLength;
	};

	NormalsDebugPass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream);

	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera) override;

	HPassData PassData;
private:
	void BuildConstantsBuffer(Hazel::GameObject* gameObject);
	void RenderItems(Hazel::TComPtr<ID3D12GraphicsCommandList> cmdList, Hazel::GameObject* goptr);


	struct alignas(16) HPerObjectData {
		glm::mat4 LocalToWorld;
		glm::mat4 NormalMatrix;
	};

	Hazel::Ref<Hazel::D3D12UploadBuffer<HPassData>> m_PassCB;
	Hazel::Ref<Hazel::D3D12UploadBuffer<HPerObjectData>> m_PerObjectCB;
};

