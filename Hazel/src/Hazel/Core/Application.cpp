#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Hazel/Core/Log.h"
#include "Hazel/Core/Input.h"

#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/Profiler/Profiler.h"

#include <glfw/glfw3.h>
#include <imgui.h>
#include <Commdlg.h>

#define USE_IMGUI 0

namespace Hazel {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationOptions& opts)
	{
		HZ_PROFILE_FUNCTION();

		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Create(WindowProps(opts.Name, opts.Width, opts.Height));
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));
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

	std::string Application::OpenFile(const std::string& filter)
	{
		OPENFILENAMEA ofn;       // common dialog box structure
		CHAR szFile[260] = { 0 };       // if using TCHAR macros

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "All\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
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
                
				D3D12Renderer::BeginFrame();
				{
                    CPUProfileBlock cpuBlock("Render");
                    GPUProfileBlock gpuBlock(D3D12Renderer::Context->DeviceResources->CommandList.Get(), "Render Total");

                    {
                        CPUProfileBlock cpuBlock("LayerStack update");
                        for (Layer* layer : m_LayerStack)
                            layer->OnUpdate(m_TimeStep);
                    }
#if USE_IMGUI
					{
                        CPUProfileBlock cpuBlock("ImGui Update");
                        GPUProfileBlock gpuBlock(D3D12Renderer::Context->DeviceResources->CommandList.Get(), "ImGui Render");
                        m_ImGuiLayer->Begin();
                        {
                            for (Layer* layer : m_LayerStack)
                                layer->OnImGuiRender();

                            ImGui::Begin("Diagnostics");
                            Profiler::GlobalProfiler.RenderStats();
                            //ImGui::Text("Frame Time: %.2f ms (%.2f fps)", m_TimeStep.GetMilliseconds(), 1000.0f / m_TimeStep.GetMilliseconds());
                            D3D12Renderer::RenderDiagnostics();
                            ImGui::End();
                        }
                        m_ImGuiLayer->End();
					}
#endif	
				}
        
				Profiler::GlobalProfiler.EndFrame(m_Window->GetWidth(), m_Window->GetHeight());
				D3D12Renderer::EndFrame();
				D3D12Renderer::Present();
			}

            for (Layer* layer : m_LayerStack)
                layer->OnFrameEnd();

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
