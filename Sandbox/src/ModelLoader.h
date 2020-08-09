#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/TextureLibrary.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/D3D12ResourceBatch.h"


struct aiScene;
struct aiNode;

class ModelLoader
{
public:
	static Hazel::TextureLibrary* TextureLibrary;
	static Hazel::Ref<Hazel::GameObject>LoadFromFile(std::string& filepath, Hazel::D3D12ResourceBatch& batch, bool swapHandeness = false);

	static void LoadScene(Hazel::Scene& scene, std::string& filepath, Hazel::D3D12ResourceBatch& batch, bool swapHandeness = false);

private:
	static void extractMaterials(const aiScene* scene, Hazel::D3D12ResourceBatch& batch, 
		std::vector<Hazel::Ref<Hazel::HMaterial>>& materials);
   
	static void processNode(aiNode* node, const aiScene* scene,
        Hazel::Ref<Hazel::GameObject> target,
        Hazel::D3D12ResourceBatch& batch,
        std::vector<Hazel::Ref<Hazel::HMaterial>>& materials);
};

