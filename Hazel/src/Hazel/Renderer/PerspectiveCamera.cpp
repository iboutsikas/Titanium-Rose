#include "hzpch.h"
#include "Hazel/Renderer/PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Hazel {

	PerspectiveCamera::PerspectiveCamera(const glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar)
		: m_Position(position), m_ProjectionMatrix(glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar)), m_ViewMatrix(1.0f), 
		  m_Pitch(0.0f), m_Yaw(-90.0f)
	{
		HZ_PROFILE_FUNCTION();
		RecalculateVectors();
		RecalculateViewMatrix();
	}

	void PerspectiveCamera::SetProjection(float width, float height, float fov, float zNear, float zFar)
	{
		HZ_PROFILE_FUNCTION();

		m_ProjectionMatrix = glm::perspectiveFovZO(glm::radians(fov), width, height, zNear, zFar);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::RecalculateViewMatrix()
	{
		HZ_PROFILE_FUNCTION();

		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::RecalculateVectors()
	{
		HZ_PROFILE_FUNCTION();

		glm::vec3 front;
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Pitch));
		front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

		m_Forward = glm::normalize(front);
		m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
		m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
	}

}