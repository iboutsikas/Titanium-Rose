#pragma once

#include "Hazel/Renderer/PerspectiveCamera.h"
#include "Hazel/Core/Timestep.h"

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/MouseEvent.h"

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
		PerspectiveCamera m_Camera;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 180.0f;
	};

}