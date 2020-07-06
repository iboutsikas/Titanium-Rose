#pragma once

#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/Core/Core.h"
#include "Hazel/ComponentSystem/HMesh.h"
#include "Hazel/Renderer/Material.h"
#include "Hazel/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"



#include <vector>
#include <string>

namespace Hazel {

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