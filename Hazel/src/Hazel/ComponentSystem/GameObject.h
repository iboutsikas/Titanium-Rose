#pragma once

#include "Hazel/ComponentSystem/Transform.h"
#include "Hazel/Core/Core.h"
#include "Hazel/ComponentSystem/HMesh.h"
#include "Hazel/ComponentSystem/Component.h"
#include "Hazel/Renderer/Material.h"
#include "Hazel/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"



#include <vector>
#include <string>

namespace Hazel {

    struct DecoupledTextureComponent
    {
        bool UseDecoupledTexture = false;
        Ref<D3D12VirtualTexture2D> VirtualTexture = nullptr;
    };

	class GameObject
	{
	public:

		~GameObject() 
		{ 

		}

		HTransform Transform;
		Hazel::Ref<HMesh> Mesh;
		Hazel::Ref<HMaterial> Material;
		DecoupledTextureComponent DecoupledComponent;

		std::vector<Hazel::Ref<Hazel::GameObject>> children;

		void AddChild(Hazel::Ref<Hazel::GameObject> child) {
			if (child.get() == this || child == nullptr)
				return;

			children.push_back(child);
			child->SetParent(this);
		}

		void Update(Timestep ts);

		std::vector<Ref<Component>> Components;

		template <typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Hazel::Component, TComponent>>>
		Ref<TComponent> AddComponent()
		{
			auto component = CreateRef<TComponent>();
			component->gameObject = this;
			Components.emplace_back(component);

			return component;
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