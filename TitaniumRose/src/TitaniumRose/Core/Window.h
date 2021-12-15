#pragma once

#include "TitaniumRose/Core/Core.h"
#include "TitaniumRose/Events/Event.h"

#include <functional>

namespace Roses {
	class GraphicsContext;
	struct WindowProps
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;

		WindowProps(const std::string& title = "Hazel Engine",
			        unsigned int width = 1280,
			        unsigned int height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
		virtual void SetTitle(const char* title) = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

		virtual void* GetNativeWindow() const = 0;
		static Scope<Window> Initialize(const WindowProps& props = WindowProps());
	};

}