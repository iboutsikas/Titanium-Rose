#pragma once
#include "Platform/D3D12/D3D12RenderPass.h"
#include "Platform/D3D12/D3D12Buffer.h"

class ClearFeedbackPass: Hazel::D3D12RenderPass<3, 1>
{
public:
	ClearFeedbackPass(Hazel::D3D12Context* ctx);

	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera) override;
	virtual void SetInput(uint32_t index, Hazel::Ref<Hazel::D3D12Texture2D> input);
private:
	struct alignas(16) HPerObjectData
	{
		uint32_t ClearValue;
		uint32_t TextureIndex;
	};

	Hazel::Ref<Hazel::D3D12UploadBuffer<HPerObjectData>> m_PerObjectCB;

};

