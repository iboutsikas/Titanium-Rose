#pragma once

#include "glm/vec3.hpp"
#include "glm/gtx/quaternion.hpp"


namespace Hazel {
	class Transform
	{
	public:

		static constexpr glm::vec3 VECTOR_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
		static constexpr glm::vec3 VECTOR_UP = glm::vec3(0.0f, 1.0f, 0.0f);
		static constexpr glm::vec3 VECTOR_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);

		enum class Space {
			Self,
			Other
		};

		Transform();

		glm::vec3 Position() { return m_Position; }
		void SetPosition(glm::vec3 value) { m_Position = value; SetDirty(); }
		void SetPosition(float x, float y, float z);

		glm::vec3 Scale() { return m_Scale; }
		void SetScale(glm::vec3 value) { m_Scale = value; SetDirty(); }
		void SetScale(float x, float y, float z);

		glm::quat Rotation() { return m_Rotation; }
		glm::mat4 RotationMatrix() { return glm::mat4_cast(m_Rotation); }
		void SetRotation(glm::quat rotation) {
			m_Rotation = glm::normalize(rotation);
			SetDirty();
		}
		void Rotate(glm::vec3 eulerAngles);
		void Rotate(glm::vec3 eulerAngles, Space relativeTo);
		void Rotate(float xAngle, float yAngle, float zAngle);
		void RotateAround(glm::vec3 axis, float angle);

		glm::vec3 Right();
		glm::vec3 Up();
		glm::vec3 Forward();

		Transform* Parent() { return m_Parent; }
		void SetParent(Transform* parent);
		void AddChild(Transform* child);
		bool HasChanged();
		glm::mat4 LocalToWorldMatrix();
		glm::mat4 WorldToLocalMatrix();

	private:
		glm::vec3 m_Position;
		glm::vec3 m_Scale;
		glm::quat m_Rotation; // Rename to orientation?
		Transform* m_Parent;
		std::vector<Transform*> m_Children;
		bool m_IsDirty;
		bool m_IsInverseDirty;

		glm::mat4 m_LocalToWorldMatrix;
		glm::mat4 m_WorldToLocalMatrix;

		void SetDirty();
	};
}