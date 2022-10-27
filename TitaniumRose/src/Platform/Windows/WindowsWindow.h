#pragma once

#include "TitaniumRose/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Roses {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;

		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		inline virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		inline virtual void* GetNativeWindow() const { return m_Window; }

		virtual void SetTitle(const char* title) override;
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}