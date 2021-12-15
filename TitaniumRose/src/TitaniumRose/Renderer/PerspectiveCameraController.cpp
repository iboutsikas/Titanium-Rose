#include "trpch.h"
#include "TitaniumRose/Renderer/PerspectiveCameraController.h"

#include "TitaniumRose/Core/Input.h"
#include "TitaniumRose/Core/KeyCodes.h"
#include "TitaniumRose/Core/Application.h"

#include "glm/gtc/quaternion.hpp"

namespace Roses {

	PerspectiveCameraController::PerspectiveCameraController(glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar)
		: m_AspectRatio(aspectRatio), 
		m_Fov(fov), 
		m_zNear(zNear), 
		m_zFar(zFar),
		m_Camera(position, fov, aspectRatio, zNear, zFar)
	{
		m_Camera.GetTransform().RotateAround(HTransform::VECTOR_UP, 180);
	}

	void PerspectiveCameraController::OnUpdate(Timestep ts)
	{
		HZ_PROFILE_FUNCTION();

		glm::vec3 cameraPosition = m_Camera.GetPosition();

		if (Input::IsKeyPressed(HZ_KEY_A))
		{
			cameraPosition -= m_Camera.GetRight() * m_CameraTranslationSpeed * glm::vec3(ts.GetSeconds());
		}
		else if (Input::IsKeyPressed(HZ_KEY_D))
		{
			cameraPosition += m_Camera.GetRight() * m_CameraTranslationSpeed * glm::vec3(ts.GetSeconds());
		}

		if (Input::IsKeyPressed(HZ_KEY_W))
		{
			cameraPosition += m_Camera.GetForward() * m_CameraTranslationSpeed * glm::vec3(ts.GetSeconds());
		}
		else if (Input::IsKeyPressed(HZ_KEY_S))
		{
			cameraPosition -= m_Camera.GetForward() * m_CameraTranslationSpeed * glm::vec3(ts.GetSeconds());
		}

		if (Input::IsKeyPressed(HZ_KEY_SPACE)) 
		{
			cameraPosition += m_Camera.GetUp() * m_CameraTranslationSpeed * glm::vec3(ts.GetSeconds());
		}
		else if (Input::IsKeyPressed(HZ_KEY_X))
		{
			cameraPosition -= m_Camera.GetUp() * m_CameraTranslationSpeed * glm::vec3(ts.GetSeconds());
		}
		

		m_Camera.SetPosition(cameraPosition);

		static bool isMouseLocked = false;

		if (Input::IsMouseButtonPressed(HZ_MOUSE_BUTTON_1)) {
			float centerX = Roses::Application::Get().GetWindow().GetWidth() / 2.0f;
			float centerY = Roses::Application::Get().GetWindow().GetHeight() / 2.0f;

			if (!isMouseLocked) {
				isMouseLocked = true;
				Input::SetCursor(false);
				Input::SetMousePosition(centerX, centerY);
			}

			auto [x, y] = Input::GetMousePosition();
			

			float deltaX = x - centerX;
			float deltaY = y - centerY;


			float yaw = deltaX * m_CameraRotationSpeed * ts.GetSeconds();
			float pitch = deltaY * m_CameraRotationSpeed * ts.GetSeconds();
			float roll = 0.0f; // maybe implement this in the future?
			m_LastMouseX = x;
			m_LastMouseY = y;
			/*glm::vec3 angles(pitch, yaw, roll);*/

			auto& tf = m_Camera.GetTransform();
			tf.RotateAround(tf.Right(), pitch);
			tf.RotateAround(HTransform::VECTOR_UP, yaw);
			Input::SetMousePosition(centerX, centerY);
		}
		else {
			if (isMouseLocked) {
				isMouseLocked = false;
				Input::SetCursor(true);
			}
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

		/*HZ_CORE_INFO("OnMouseMoved: <{}, {}>", m_LastMouseX, m_LastMouseY);*/

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