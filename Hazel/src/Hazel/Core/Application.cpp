#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Hazel/Core/Log.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Core/SystemTime.h"

#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/Profiler/Profiler.h"

#include <glfw/glfw3.h>
#include <imgui.h>
#include <Commdlg.h>

#define USE_IMGUI 1

static constexpr float FixedTime = 1.0f / 60.0f;

namespace Hazel {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationOptions& opts)
	{
		HZ_PROFILE_FUNCTION();

		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Initialize(WindowProps(opts.Name, opts.Width, opts.Height));
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));
	}

	Application::~Application()
	{
		HZ_PROFILE_FUNCTION();

		Profiler::Shutdown();
		D3D12Renderer::Shutdown();
	}

	void Application::Init()
	{
		SystemTime::Initialize();
		D3D12Renderer::Init();
		Profiler::Initialize();
#if USE_IMGUI
		m_ImGuiLayer = ImGuiLayer::Initialize();
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
			Profiler::Update();

			if (!m_Minimized)
			{	
                D3D12Renderer::BeginFrame();

				

				Profiler::BeginBlock("Frame Total", D3D12Renderer::Context->DeviceResources->MainCommandList);
				//D3D12Renderer::ShaderLibrary->Update();

				{
                    Profiler::BeginBlock("OnUpdate", D3D12Renderer::Context->DeviceResources->MainCommandList);
                    for (Layer* layer : m_LayerStack)
                        layer->OnUpdate(m_TimeStep);
                    Profiler::EndBlock(D3D12Renderer::Context->DeviceResources->MainCommandList);

					GraphicsContext& gfxContext = GraphicsContext::Begin("Render");
					//Profiler::BeginBlock("OnRender", D3D12Renderer::Context->DeviceResources->MainCommandList);
                    for (Layer* layer : m_LayerStack)
                        layer->OnRender(m_TimeStep, gfxContext);
					gfxContext.Finish(true);
					//Profiler::EndBlock(D3D12Renderer::Context->DeviceResources->MainCommandList);

#if USE_IMGUI
                    //Profiler::BeginBlock("OnImGuiRender", D3D12Renderer::Context->DeviceResources->MainCommandList);
					GraphicsContext& uiContext = GraphicsContext::Begin("UI");
					m_ImGuiLayer->Begin(uiContext);
					{
						for (Layer* layer : m_LayerStack)
							layer->OnImGuiRender(uiContext);

						ImGui::Begin("Diagnostics");
						Profiler::Render();
						D3D12Renderer::RenderDiagnostics();
						ImGui::End();
					}
					m_ImGuiLayer->End(uiContext);
                    //Profiler::EndBlock(D3D12Renderer::Context->DeviceResources->MainCommandList);

#endif	
				}
				Profiler::EndBlock(D3D12Renderer::Context->DeviceResources->MainCommandList);
				
				D3D12Renderer::Present();

                for (Layer* layer : m_LayerStack)
                    layer->OnFrameEnd();
			}		

			m_Window->OnUpdate();

			float time = (float)glfwGetTime();
			m_TimeStep = /*FixedTime;*/ time - m_LastFrameTime;
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
