#pragma once

#include "Hazel/Core/Layer.h"

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"

#include "Platform/D3D12/CommandContext.h"

namespace Hazel {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void Begin(GraphicsContext& context) = 0;
		virtual void End(GraphicsContext& context) = 0;

		static ImGuiLayer* Initialize();
	protected:
		float m_Time = 0.0f;
	};

}