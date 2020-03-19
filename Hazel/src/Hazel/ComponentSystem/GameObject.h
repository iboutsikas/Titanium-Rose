#pragma once

#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"



#include <vector>



namespace Hazel {
	class Mesh
	{
	public:
		Hazel::Ref<Hazel::D3D12VertexBuffer> vertexBuffer;
		Hazel::Ref<Hazel::D3D12IndexBuffer> indexBuffer;
		Hazel::Ref<Hazel::D3D12Texture2D> diffuseTexture;
	};

	

	class GameObject
	{
	public:
		Transform transform;
		Mesh mesh;
		std::vector<GameObject*> children;

		void AddChild(GameObject* child) {
			if (child == this || child == nullptr)
				return;

			children.push_back(child);
			child->SetParent(this);
		}

		void SetParent(GameObject* parent) {
			if (parent == this || parent == nullptr)
				return;
			this->transform.SetParent(&parent->transform);
			this->m_Parent = parent;
		}

	private:
		GameObject* m_Parent = nullptr;
	};
}