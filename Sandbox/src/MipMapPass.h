#pragma once
#include "Platform/D3D12/D3D12RenderPass.h"


class MipMapPass: public Hazel::D3D12RenderPass<1, 1>
{
public:
	struct alignas(16) HPassData {
		uint32_t	SourceLevel;
		uint32_t	Levels;
		glm::vec4	TexelSize;
	};
	
	MipMapPass(Hazel::D3D12Context* ctx);

	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera) override;

	HPassData PassData;

private:

};

