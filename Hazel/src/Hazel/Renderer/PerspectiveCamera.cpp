#include "hzpch.h"
#include "Hazel/Renderer/PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Hazel {

	PerspectiveCamera::PerspectiveCamera(const glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar) : 
		m_ProjectionMatrix(glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar)), 
		m_ViewMatrix(1.0f)
	{
		HZ_PROFILE_FUNCTION();
		m_Transform.SetPosition(position);
		RecalculateViewMatrix();
	}

	void PerspectiveCamera::SetProjection(float width, float height, float fov, float zNear, float zFar)
	{
		HZ_PROFILE_FUNCTION();

		m_ProjectionMatrix = glm::perspectiveFovZO(glm::radians(fov), width, height, zNear, zFar);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	Ray PerspectiveCamera::ScreenspaceToRay(float x, float y)
	{
		// DX12 Z is [0, 1] in clip space, not [-1, 1]
        glm::vec4 mouseClipPosition = { x, y, 0.0f, 1.0f };
		auto inverseProj = glm::inverse(m_ProjectionMatrix);
		auto inverseView = glm::inverse(glm::mat3(m_ViewMatrix));

		glm::vec4 ray = inverseProj * mouseClipPosition;
		glm::vec3 rayPos = m_Transform.Position();
		glm::vec3 dir = inverseView * glm::vec3(ray);


		return Ray(rayPos, dir);
	}

	void PerspectiveCamera::RecalculateViewMatrix()
	{
		HZ_PROFILE_FUNCTION();

		//glm::mat3 rZyx(glm::mat3_cast(m_Rotation));

		//m_Forward = rZyx * WORLD_FORWARD;
		//m_Right = rZyx * WORLD_RIGHT;
		//m_Up = rZyx * WORLD_UP;


		//auto m_ViewMatrix1 = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);

		//m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix1;
		auto pos = m_Transform.Position();
		auto forward = m_Transform.Forward();
		auto up = m_Transform.Up();
		auto right = m_Transform.Right();
		m_ViewMatrix = glm::lookAt(pos, pos + forward, up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}