#pragma once

#include "Hazel/ImGui/ImGuiLayer.h"

namespace Hazel {

	class OpenGLImGuiLayer : public ImGuiLayer {
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void Begin() override;
		virtual void End() override;
	};
}