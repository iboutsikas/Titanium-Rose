#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Core/Math/Ray.h"


namespace Hazel {

	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(const glm::vec3& position, float fov, float aspectRatio, float zNear, float zFar);

		void SetProjection(float width, float height, float fov, float zNear, float zFar);

		const glm::vec3 GetPosition() { return m_Transform.Position(); }
		void SetPosition(const glm::vec3& position) { m_Transform.SetPosition(position); }

		const glm::quat GetRotation() { return m_Transform.Rotation(); }
		void SetRotation(const glm::quat& rotation) { m_Transform.SetRotation(rotation); }

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
		
		glm::vec3 GetForward() { return m_Transform.Forward(); }
		glm::vec3 GetRight() { return m_Transform.Right(); }
		glm::vec3 GetUp() { return m_Transform.Up(); }

		HTransform& GetTransform() { return m_Transform; }

		Ray ScreenspaceToRay(float x, float y);

		void RecalculateViewMatrix();
	private:
		HTransform m_Transform;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ViewProjectionMatrix;
	};;

}
