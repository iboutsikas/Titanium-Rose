#pragma once

#include "TitaniumRose/Core/Core.h"
#include "TitaniumRose/Core/Window.h"
#include "TitaniumRose/Core/LayerStack.h"
#include "TitaniumRose/Core/Timestep.h"

#include "TitaniumRose/Events/Event.h"
#include "TitaniumRose/Events/ApplicationEvent.h"

#include "TitaniumRose/ImGui/ImGuiLayer.h"

#include "cxxopts/include/cxxopts.hpp"

int main(int argc, char** argv);

namespace Roses {

	class Application
	{
	public:

		struct ApplicationOptions
		{
			uint32_t Width;
			uint32_t Height;
			std::string Name;
			bool UseFixedTime;
		};

		Application(const ApplicationOptions& opts = { 1280, 720, "Hazel Engine", false });
		virtual ~Application();

		void Init();

		void OnEvent(Event& e);
		virtual void OnInit(cxxopts::ParseResult& options) = 0;
		virtual void AddApplicationOptions(cxxopts::Options& options) = 0;

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		std::string OpenFile(const std::string& filter);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	protected:

		bool m_UseFixedTimestep;

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