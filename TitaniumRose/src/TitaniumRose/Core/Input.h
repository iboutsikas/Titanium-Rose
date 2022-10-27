#pragma once

#include "TitaniumRose/Core/Core.h"
#include "TitaniumRose/Core/KeyCodes.h"
#include "TitaniumRose/Core/MouseCodes.h"

namespace Roses {

	class Input
	{
	protected:
		Input() = default;
	public:
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		inline static bool IsKeyPressed(KeyCode key) { return s_Instance->IsKeyPressedImpl(key); }

		inline static bool IsMouseButtonPressed(MouseCode button) { return s_Instance->IsMouseButtonPressedImpl(button); }
		inline static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
		inline static void SetMousePosition(float x, float y) { return s_Instance->SetMousePositionImp(x, y); }
		inline static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
		inline static float GetMouseY() { return s_Instance->GetMouseYImpl(); }
		inline static void SetCursor(bool enable) { s_Instance->SetCursorImpl(enable); }
		static Scope<Input> Initialize();
	protected:
		virtual bool IsKeyPressedImpl(KeyCode key) = 0;

		virtual bool IsMouseButtonPressedImpl(MouseCode button) = 0;
		virtual std::pair<float, float> GetMousePositionImpl() = 0;
		virtual void SetMousePositionImp(float x, float y) = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;
		virtual void SetCursorImpl(bool enable) = 0;
	private:
		static Scope<Input> s_Instance;
	};
}