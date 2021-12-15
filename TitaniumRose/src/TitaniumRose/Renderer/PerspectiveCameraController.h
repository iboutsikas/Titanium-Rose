#pragma once

#include "TitaniumRose/Core/Timestep.h"
#include "TitaniumRose/Events/ApplicationEvent.h"
#include "TitaniumRose/Events/MouseEvent.h"
#include "TitaniumRose/Renderer/PerspectiveCamera.h"

#include "glm/gtc/quaternion.hpp"

namespace Roses {

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

		float m_CameraTranslationSpeed = 15.0f, m_CameraRotationSpeed = 10.0f;
	};

}