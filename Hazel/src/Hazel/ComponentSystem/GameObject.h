#pragma once

#include "Hazel/Core/Core.h"
#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"

#include "glm/vec3.hpp"
#include "glm/gtx/quaternion.hpp"

#include <vector>

constexpr glm::vec3 VECTOR_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 VECTOR_UP = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 VECTOR_FORWARD = glm::vec3(0.0f, 0.0f, -1.0f);

namespace Hazel {
	class Mesh
	{
	public:
		Hazel::Ref<Hazel::D3D12VertexBuffer> vertexBuffer;
		Hazel::Ref<Hazel::D3D12IndexBuffer> indexBuffer;
		Hazel::Ref<Hazel::D3D12Texture2D> diffuseTexture;
	};

	class Transform
	{
	public:

		enum class Space {
			Self,
			Other
		};

		glm::vec3 Position() { return m_Position; }
		void Position(glm::vec3 value) { m_Position = value; SetDirty(); }
		void Position(float x, float y, float z) { 
			m_Position.x = x; 
			m_Position.y = y;
			m_Position.z = z;
			SetDirty();
		}

		glm::vec3 Scale() { return m_Scale; }
		void Scale(glm::vec3 value) { m_Scale = value; SetDirty(); }
		void Scale(float x, float y, float z) {
			m_Scale.x = x;
			m_Scale.y = y;
			m_Scale.z = z;
			SetDirty();
		}

		glm::quat Rotation() { return m_Rotation; }
		glm::mat4 RotationMatrix() { return glm::mat4_cast(m_Rotation); }

		void Rotate(glm::vec3 eulerAngles) {
			Rotate(eulerAngles, Space::Self);
		}

		void Rotate(glm::vec3 eulerAngles, Space relativeTo) {
			glm::quat quaternion(eulerAngles);
			if (relativeTo == Space::Self) {
				this->m_Rotation = m_Rotation * quaternion;
			}
			else {
				this->m_Rotation = m_Rotation * glm::inverse(m_Rotation) * quaternion * m_Rotation;
			}
		}

		void Rotate(float xAngle, float yAngle, float zAngle) {
			glm::vec3 angles(xAngle, yAngle, zAngle);
			Rotate(angles, Space::Self);
		}

		void RotateAround(glm::vec3 axis, float angle) {
			auto rotQuat = glm::angleAxis(angle, axis);

			m_Rotation = m_Rotation * rotQuat;
		}


		glm::vec3 Right() { return VECTOR_RIGHT * this->m_Rotation; }
		glm::vec3 Up() { return VECTOR_UP * this->m_Rotation; }
		glm::vec3 Forward() { return VECTOR_UP * this->m_Rotation; }

		Transform* Parent() { return m_Parent; }
		void SetParent(Transform* parent) {
			if (this != parent) {
				this->m_Parent = parent;
				SetDirty();
			}
		}

		bool HasChanged() {
			if (m_Parent != nullptr && m_Parent->HasChanged())
				return true;

			return m_IsDirty;
		}

		glm::mat4 LocalToWorldMatrix() {

			if (m_IsDirty) {
				glm::mat4 local(1);
				glm::mat4 rot = glm::mat4_cast(m_Rotation);

				local = glm::scale(local, m_Scale);
				local = rot * local;
				local = glm::translate(local, m_Position);

				if (m_Parent != nullptr) {
					local = m_Parent->LocalToWorldMatrix() * local;
				}
				m_IsDirty = false;
				m_LocalToWorldMatrix = local;
			}

			return m_LocalToWorldMatrix;
		}

		glm::mat4 WorldToLocalMatrix() {
			if (m_IsInverseDirty) {
				m_WorldToLocalMatrix = glm::inverse(LocalToWorldMatrix());
				m_IsInverseDirty = false;
			}
			return m_WorldToLocalMatrix;
		}

	private:
		glm::vec3 m_Position;
		glm::vec3 m_Scale;
		glm::quat m_Rotation; // Rename to orientation?
		Transform* m_Parent;
		bool m_IsDirty;
		bool m_IsInverseDirty;
		
		glm::mat4 m_LocalToWorldMatrix;
		glm::mat4 m_WorldToLocalMatrix;

		void SetDirty() {
			if (!m_IsDirty) {
				m_IsDirty = m_IsInverseDirty = true;
			}
		}
	};

	class GameObject
	{
	public:
		Transform transform;
		Mesh mesh;
		std::vector<GameObject> children;
	};
}