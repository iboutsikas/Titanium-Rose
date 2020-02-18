#include "hzpch.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"

#include "Hazel/Core/Input.h"
#include "Hazel/Core/KeyCodes.h"

namespace Hazel {

	PerspectiveCameraController::PerspectiveCameraController(glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar)
		: m_AspectRatio(aspectRatio), m_Fov(fov), m_zNear(zNear), m_zFar(zFar), m_Camera(position, fov, aspectRatio, zNear, zFar)
	{
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

		if (Input::IsMouseButtonPressed(MouseCode::Button2)) 
		{
			HZ_INFO("Mouse Button2 pressed");
			//e.
			//xoffset *= MouseSensitivity;
			//yoffset *= MouseSensitivity;

			//Yaw += xoffset;
			//Pitch += yoffset;

			//// Make sure that when pitch is out of bounds, screen doesn't get flipped
			//if (constrainPitch)
			//{
			//	if (Pitch > 89.0f)
			//		Pitch = 89.0f;
			//	if (Pitch < -89.0f)
			//		Pitch = -89.0f;
			//}

			//// Update Front, Right and Up Vectors using the updated Euler angles
			//updateCameraVectors();
		}
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