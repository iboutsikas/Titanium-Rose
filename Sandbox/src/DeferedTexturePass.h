#pragma once
#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/PerspectiveCamera.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12RenderPass.h"
#include "Platform/D3D12/D3D12Texture.h"

class DeferedTexturePass : public Hazel::D3D12RenderPass<1, 1>
{
public:
	struct alignas(16) PassData {
		glm::mat4 ViewProjection;
		glm::vec4 AmbientLight;
		glm::vec4 DirectionalLight;
		glm::vec3 DirectionalLightPosition;
		float     __padding1;
		glm::vec3 CameraPosition;
		float     AmbientIntensity;
	};

	DeferedTexturePass(Hazel::D3D12Context* ctx, Hazel::D3D12Shader::PipelineStateStream& pipelineStream);

	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera) override;
	virtual void SetInput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> input) override;
	virtual void SetOutput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> output) override;

	PassData HPassData;

private:
	struct alignas(16) PerObjectData {
		glm::mat4 ModelMatrix;
		glm::mat4 NormalsMatrix;
		float     Glossiness;
	};
	
	Hazel::Ref<Hazel::D3D12UploadBuffer<PassData>> m_PassCB;
	Hazel::Ref<Hazel::D3D12UploadBuffer<PerObjectData>> m_PerObjectCB;
	Hazel::TComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	Hazel::D3D12Context* m_Context;
};

