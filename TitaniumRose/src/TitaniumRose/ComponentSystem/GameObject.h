#pragma once

#include "TitaniumRose/ComponentSystem/Transform.h"
#include "TitaniumRose/Core/Core.h"
#include "TitaniumRose/ComponentSystem/HMesh.h"
#include "TitaniumRose/ComponentSystem/Component.h"
#include "TitaniumRose/Renderer/Material.h"
#include "TitaniumRose/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Buffer.h"
#include "Platform/D3D12/D3D12Texture.h"



#include <vector>
#include <string>

namespace Roses {

    struct DecoupledTextureComponent
    {
        bool UseDecoupledTexture = false;
		bool OverwriteRefreshRate = false;
		uint64_t LastFrameUpdated = 0;
		uint64_t UpdateFrequency = 1;
        Ref<VirtualTexture2D> VirtualTexture = nullptr;
    };

	class HGameObject
	{
	public:

		~HGameObject() 
		{ 

		}

		HTransform Transform;
		Roses::Ref<HMesh> Mesh;
		Roses::Ref<HMaterial> Material;
		DecoupledTextureComponent DecoupledComponent;

		std::vector<Roses::Ref<Roses::HGameObject>> children;

		void AddChild(Roses::Ref<Roses::HGameObject> child) {
			if (child.get() == this || child == nullptr)
				return;

			children.push_back(child);
			child->SetParent(this);
		}

		void Update(Timestep ts);

		std::vector<Ref<Component>> Components;

		template <typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Roses::Component, TComponent>>>
		Ref<TComponent> AddComponent()
		{
			auto component = CreateRef<TComponent>();
			component->gameObject = this;
			Components.emplace_back(component);

			return component;
		}

		std::string Name;

		uint32_t ID = -1;
		

	private:
		HGameObject* m_Parent = nullptr;
		
		void SetParent(HGameObject* parent) {
			if (parent == this || parent == nullptr)
				return;
			this->Transform.SetParent(&parent->Transform);
			this->m_Parent = parent;
		}
	};
}