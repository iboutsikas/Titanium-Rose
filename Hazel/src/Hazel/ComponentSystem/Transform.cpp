#include "hzpch.h"

#include "Hazel/ComponentSystem/Transform.h"

namespace Hazel {
	

	Transform::Transform() :
		m_Position(glm::vec3(0.0f)),
		m_Rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
		m_Scale(glm::vec3(1.0f)),
		m_IsDirty(true),
		m_IsInverseDirty(true),
		m_Parent(nullptr)
	{

	}

	void Transform::SetPosition(float x, float y, float z) {
		m_Position.x = x;
		m_Position.y = y;
		m_Position.z = z;
		SetDirty();
	}

	void Transform::SetScale(float x, float y, float z) {
		m_Scale.x = x;
		m_Scale.y = y;
		m_Scale.z = z;
		SetDirty();
	}

	void Transform::Rotate(glm::vec3 eulerAngles) {
		Transform::Rotate(eulerAngles, Space::Self);
	}

	void Transform::Rotate(glm::vec3 eulerAngles, Space relativeTo) {
		glm::quat quaternion(eulerAngles);
		if (relativeTo == Space::Self) {
			this->m_Rotation = quaternion * m_Rotation;
		}
		else {
			this->m_Rotation = m_Rotation * glm::inverse(m_Rotation) * quaternion * m_Rotation;
		}
		this->m_Rotation = glm::normalize(this->m_Rotation);
		SetDirty();
	}

	void Transform::Rotate(float xAngle, float yAngle, float zAngle) {
		glm::vec3 angles(xAngle, yAngle, zAngle);
		Rotate(angles, Space::Self);
	}

	// angle is in degrees
	void Transform::RotateAround(glm::vec3 axis, float angle) {
		auto rotQuat = glm::angleAxis(glm::radians(angle), axis);

		m_Rotation = glm::normalize(m_Rotation * rotQuat);
		SetDirty();
	}

	void Transform::LookAt(const glm::vec3& point, const glm::vec3& up)
	{
		auto normUp = Up();
		auto dir = glm::normalize(point - m_Position);

		float cr = glm::dot(dir, Forward());
		glm::mat4 RotationMatrix = glm::lookAt(m_Position, point, normUp);
		m_Rotation = glm::normalize(glm::toQuat(RotationMatrix));
		
		auto thing = glm::quatLookAt(dir, normUp);
		SetDirty();
	}

	glm::vec3 Transform::EulerAngles()
	{
		return glm::vec3();
	}

	glm::vec3 Transform::Right() { return glm::normalize(VECTOR_RIGHT * this->m_Rotation); }
	glm::vec3 Transform::Up() { return glm::normalize(VECTOR_UP * this->m_Rotation); }
	glm::vec3 Transform::Forward() { return glm::normalize(VECTOR_FORWARD * this->m_Rotation); }

	void Transform::SetParent(Transform* parent) {
		if (this != parent) {
			this->m_Parent = parent;
			this->m_Parent->AddChild(this);
			SetDirty();
		}
	}

	void Transform::AddChild(Transform* child)
	{
		if (this != child) {
			this->m_Children.push_back(child);
		}
	}

	bool Transform::HasChanged() {
		if (m_Parent != nullptr && m_Parent->HasChanged())
			return true;

		return m_IsDirty;
	}

	glm::mat4 Transform::LocalToWorldMatrix() {

		if (m_IsDirty) {
			glm::mat4 local(1);
			glm::mat4 r = glm::mat4_cast(m_Rotation);

			auto s = glm::scale(local, m_Scale);
			//local = rot * local;
			auto t = glm::translate(local, m_Position);

			local = t * r * s;
			if (m_Parent != nullptr) {
				local = m_Parent->LocalToWorldMatrix() * local;
			}

			m_IsDirty = false;
			m_LocalToWorldMatrix = local;
		}
		
		return m_LocalToWorldMatrix;
	}

	glm::mat4 Transform::WorldToLocalMatrix() {
		if (m_IsInverseDirty) {
			m_WorldToLocalMatrix = glm::inverse(LocalToWorldMatrix());
			m_IsInverseDirty = false;
		}
		return m_WorldToLocalMatrix;
	}

	void Transform::SetDirty() {
		if (!m_IsDirty) {
			m_IsDirty = m_IsInverseDirty = true;
			for (auto& child : m_Children) {
				child->SetDirty();
			}
		}
	}
}