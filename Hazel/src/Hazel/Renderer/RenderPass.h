#pragma once

#include "Hazel/Core/Core.h"

#include "Hazel/Renderer/Shader.h"

namespace Hazel {
	enum class RenderPassOutputType {
		GBUFER,
		TEXTURE
	};

	class RenderPass
	{
		// Set Opacity
		// Set Inputs
		// Set Outputs
	public:
		RenderPass();
		virtual ~RenderPass();

		virtual void EnableGBuffer() = 0;
		virtual void AddOutput(uint32_t width, uint32_t height) = 0;
		virtual void Finalize() = 0;
		virtual void SetShader(Ref<Shader> shader) = 0;

		static Ref<RenderPass> Create();

	};
}

