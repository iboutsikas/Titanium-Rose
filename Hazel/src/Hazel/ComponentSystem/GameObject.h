#pragma once

#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"



#include <vector>



namespace Hazel {
	struct HMesh
	{
		Hazel::Ref<Hazel::D3D12VertexBuffer> vertexBuffer;
		Hazel::Ref<Hazel::D3D12IndexBuffer> indexBuffer;
	};

	struct HMaterial {
		uint32_t TextureId;
		float Glossines;
		uint32_t cbIndex;
	};

	class GameObject
	{
	public:
		HTransform Transform;
		Hazel::Ref<HMesh> Mesh;
		Hazel::Ref<HMaterial> Material;

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
			this->Transform.SetParent(&parent->Transform);
			this->m_Parent = parent;
		}

	private:
		GameObject* m_Parent = nullptr;
	};
}