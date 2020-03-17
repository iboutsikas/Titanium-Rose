#pragma once
#include "Platform/D3D12/D3D12RenderPass.h"
#include "Platform/D3D12/D3D12Texture.h"

class DeferedTexturePass : public Hazel::D3D12RenderPass
{
public:
	DeferedTexturePass(Hazel::D3D12Context* ctx);

	// Inherited via D3D12RenderPass
	virtual void Update(Hazel::Timestep ts) override;
	virtual void Process(Hazel::D3D12Context* ctx) override;
	virtual void Recompile(Hazel::D3D12Context* ctx) override;

	Hazel::Ref<Hazel::D3D12Texture2D> Target;
};

