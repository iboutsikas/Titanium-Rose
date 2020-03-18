#include "hzpch.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"

#include "Hazel/Core/Input.h"
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/Application.h"

#include "glm/gtc/quaternion.hpp"

namespace Hazel {

	PerspectiveCameraController::PerspectiveCameraController(glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar)
		: m_AspectRatio(aspectRatio), m_Fov(fov), m_zNear(zNear), m_zFar(zFar), m_Camera(position, fov, aspectRatio, zNear, zFar)
	{
		m_CameraQuat = m_Camera.GetRotation(); //glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	}

	void PerspectiveCameraController::OnUpdate(Timestep ts)
	{
		HZ_PROFILE_FUNCTION();

		if (Input::IsKeyPressed(HZ_KEY_A))
		{
			m_CameraPosition -= m_Camera.GetRight() * m_CameraTranslationSpeed * glm::vec3(ts);
		}
		else if (Input::IsKeyPressed(HZ_KEY_D))
		{
			m_CameraPosition += m_Camera.GetRight() * m_CameraTranslationSpeed * glm::vec3(ts);
		}

		if (Input::IsKeyPressed(HZ_KEY_W))
		{
			m_CameraPosition += m_Camera.GetForward() * m_CameraTranslationSpeed * glm::vec3(ts);
		}
		else if (Input::IsKeyPressed(HZ_KEY_S))
		{
			m_CameraPosition -= m_Camera.GetForward() * m_CameraTranslationSpeed * glm::vec3(ts);
		}

		if (Input::IsKeyPressed(HZ_KEY_SPACE)) 
		{
			m_CameraPosition += m_Camera.GetUp() * m_CameraTranslationSpeed * glm::vec3(ts);
		}
		else if (Input::IsKeyPressed(HZ_KEY_X))
		{
			m_CameraPosition -= m_Camera.GetUp() * m_CameraTranslationSpeed * glm::vec3(ts);
		}
		

		m_Camera.SetPosition(m_CameraPosition);

		if (Input::IsMouseButtonPressed(HZ_MOUSE_BUTTON_1)) {
			Input::SetCursor(false);

			//Vector2f centerPosition = new Vector2f(Window.GetWidth() / 2, Window.GetHeight() / 2);

			float centerX = Hazel::Application::Get().GetWindow().GetWidth() / 2.0f;
			float centerY = Hazel::Application::Get().GetWindow().GetHeight() / 2.0f;

			auto [x, y] = Input::GetMousePosition();

			float deltaX = x - m_LastMouseX;
			float deltaY = y - m_LastMouseY;
			

			float yaw =  deltaX * m_CameraRotationSpeed * ts;
			float pitch = deltaY* m_CameraRotationSpeed* ts;
			float roll = 0.0f; // maybe implement this in the future?
			m_LastMouseX = x;
			m_LastMouseY = y;

			glm::quat rotQuat = glm::quat(glm::vec3(-pitch, -yaw, roll));
			m_CameraQuat = rotQuat * m_CameraQuat;
			m_CameraQuat = glm::normalize(m_CameraQuat);

			m_Camera.SetRotation(m_CameraQuat);
		}
		else {
			Input::SetCursor(true);
		}

		m_Camera.RecalculateViewMatrix();
	}

	void PerspectiveCameraController::OnEvent(Event& e)
	{
		HZ_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(HZ_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));
		dispatcher.Dispatch<MouseMovedEvent>(HZ_BIND_EVENT_FN(PerspectiveCameraController::OnMouseMoved));
	}

	bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		HZ_PROFILE_FUNCTION();

		/*m_ZoomLevel -= e.GetYOffset() * 0.25f;
		m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);*/
		return false;
	}

	bool PerspectiveCameraController::OnMouseMoved(MouseMovedEvent& e)
	{
		HZ_PROFILE_FUNCTION();

		m_LastMouseX = e.GetX();
		m_LastMouseY = e.GetY();

		return false;
	}

	bool PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		HZ_PROFILE_FUNCTION();

		m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
		m_Camera.SetProjection((float)e.GetWidth(), (float)e.GetHeight(), m_Fov, m_zNear, m_zFar);
		return false;
	}

}