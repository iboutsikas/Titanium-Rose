#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"

#include "Platform/D3D12/CommandContext.h"

namespace Hazel {

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