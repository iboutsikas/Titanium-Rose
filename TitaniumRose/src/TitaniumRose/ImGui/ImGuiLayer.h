#pragma once

#include "TitaniumRose/Core/Layer.h"

#include "TitaniumRose/Events/ApplicationEvent.h"
#include "TitaniumRose/Events/KeyEvent.h"
#include "TitaniumRose/Events/MouseEvent.h"

#include "Platform/D3D12/CommandContext.h"

namespace Roses {

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