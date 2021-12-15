#pragma once

#include "TitaniumRose/Core/Core.h"
#include "TitaniumRose/Core/Timestep.h"
#include "TitaniumRose/Events/Event.h"

namespace Roses {
	class GraphicsContext;

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnRender(Timestep ts, GraphicsContext& gfxContext) {}
		virtual void OnImGuiRender(GraphicsContext& uiContext) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnFrameEnd() {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}