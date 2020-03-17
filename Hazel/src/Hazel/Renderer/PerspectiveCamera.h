#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Hazel {

	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(const glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar);

		void SetProjection(float width, float height, float fov, float zNear, float zFar);

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position) { m_Position = position; }

		const glm::quat& GetRotation() const { return m_Rotation; }
		void SetRotation(const glm::quat& rotation) { m_Rotation = rotation; }

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { 
			return m_ViewProjectionMatrix; 
		}
		const glm::vec3& GetForward() const { return m_Forward; }
		const glm::vec3& GetRight() const { return m_Right; }
		const glm::vec3& GetUp() const { return m_Up; }

		void RecalculateViewMatrix();
	private:
		void RecalculateVectors();
	private:
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_Forward = { 0.0f, 0.0f, 0.1f };
		glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Right;
		glm::quat m_Rotation;
		float m_Yaw;
		float m_Pitch;
	};

}
