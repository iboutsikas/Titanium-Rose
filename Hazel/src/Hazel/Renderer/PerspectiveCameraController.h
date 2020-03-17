#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/PerspectiveCamera.h"

#include "glm/gtc/quaternion.hpp"

namespace Hazel {

	class PerspectiveCameraController
	{
	public:
		PerspectiveCameraController(glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		PerspectiveCamera& GetCamera() { return m_Camera; }
		const PerspectiveCamera& GetCamera() const { return m_Camera; }

	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
	private:
		float m_AspectRatio;
		float m_Fov;
		float m_zNear;
		float m_zFar;
		float m_LastMouseX;
		float m_LastMouseY;
		PerspectiveCamera m_Camera;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::quat m_CameraQuat;
		float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 0.1f;
	};

}