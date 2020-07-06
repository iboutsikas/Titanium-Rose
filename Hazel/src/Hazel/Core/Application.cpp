#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Hazel/Core/Log.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Platform/D3D12/D3D12Renderer.h"


#include <glfw/glfw3.h>


#define USE_IMGUI 1

namespace Hazel {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		HZ_PROFILE_FUNCTION();

		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));

		// TODO: Depracate this. We are no longer using it
		RendererAPI::SetAPI(RendererAPI::API::D3D12);
	}

	Application::~Application()
	{
		HZ_PROFILE_FUNCTION();


		D3D12Renderer::Shutdown();
	}

	void Application::Init()
	{
		D3D12Renderer::Init();
#if USE_IMGUI
		m_ImGuiLayer = ImGuiLayer::Create();
		PushOverlay(m_ImGuiLayer);
#endif

	}

	void Application::PushLayer(Layer* layer)
	{
		HZ_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		HZ_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		HZ_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			if (!m_Minimized)
			{
				D3D12Renderer::NewFrame();
				{
					HZ_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(m_TimeStep);
				}
#if USE_IMGUI
				m_ImGuiLayer->Begin();
				{
					HZ_PROFILE_SCOPE("LayerStack OnImGuiRender");

					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
#endif	
				D3D12Renderer::Present();
			}
			m_Window->OnUpdate();

			float time = (float)glfwGetTime();
			m_TimeStep = time - m_LastFrameTime;
			m_LastFrameTime = time;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		HZ_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		D3D12Renderer::ResizeViewport(0, 0, e.GetWidth(), e.GetHeight());

		return false;
	}

}
