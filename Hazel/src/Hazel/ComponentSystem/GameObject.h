#pragma once

#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"



#include <vector>
#include <string>


namespace Hazel {
	struct HMesh
	{
		Hazel::Ref<Hazel::D3D12VertexBuffer> vertexBuffer;
		Hazel::Ref<Hazel::D3D12IndexBuffer> indexBuffer;
		Hazel::Ref<Hazel::D3D12Texture2D> diffuseTexture;
		Hazel::Ref<Hazel::D3D12Texture2D> normalMap;
	};

	struct HMaterial {
		uint32_t TextureId;
		float Glossines;
		uint32_t cbIndex;
		glm::vec4 Color;
	};

	class GameObject
	{
	public:

		HTransform Transform;
		Hazel::Ref<HMesh> Mesh;
		Hazel::Ref<HMaterial> Material;

		std::vector<Hazel::Ref<Hazel::GameObject>> children;

		void AddChild(Hazel::Ref<Hazel::GameObject> child) {
			if (child.get() == this || child == nullptr)
				return;

			children.push_back(child);
			child->SetParent(this);
		}

		std::string Name;
		

	private:
		GameObject* m_Parent = nullptr;
		
		void SetParent(GameObject* parent) {
			if (parent == this || parent == nullptr)
				return;
			this->Transform.SetParent(&parent->Transform);
			this->m_Parent = parent;
		}
	};
}