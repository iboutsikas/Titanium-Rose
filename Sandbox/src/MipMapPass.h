#pragma once
#include "Platform/D3D12/D3D12RenderPass.h"


class MipMapPass: public Hazel::D3D12RenderPass<1, 1>
{
public:
	struct alignas(16) HPassData {
		glm::vec4	TexelSize;
		uint32_t	SourceLevel;
		uint32_t	Levels;
		uint64_t    __padding;
	};
	
	MipMapPass(Hazel::D3D12Context* ctx);

	virtual void Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera) override;

	HPassData PassData;

private:

};

