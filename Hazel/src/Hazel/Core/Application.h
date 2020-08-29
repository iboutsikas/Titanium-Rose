#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/Window.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Core/Timestep.h"

#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"

#include "Hazel/ImGui/ImGuiLayer.h"

int main(int argc, char** argv);

namespace Hazel {

	class Application
	{
	public:

		struct ApplicationOptions
		{
			uint32_t Width;
			uint32_t Height;
			std::string Name;
		};

		Application(const ApplicationOptions& opts = { 1280, 720, "Hazel Engine" });
		virtual ~Application();

		void Init();

		void OnEvent(Event& e);
		virtual void OnInit() = 0;


		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		std::string OpenFile(const std::string& filter);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		Timestep m_TimeStep;

		float m_LastFrameTime = 0.0f;
	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}