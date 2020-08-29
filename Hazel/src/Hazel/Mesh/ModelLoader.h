#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/TextureLibrary.h"
#include "Hazel/Scene/Scene.h"

#include "Platform/D3D12/D3D12ResourceBatch.h"


struct aiScene;
struct aiNode;

namespace Hazel
{
    class ModelLoader
    {
    public:
        static Ref<GameObject>LoadFromFile(std::string& filepath, D3D12ResourceBatch& batch, bool swapHandeness = false);

        static void LoadScene(Scene& scene, std::string& filepath, D3D12ResourceBatch& batch, bool swapHandeness = false);

    private:
        static void extractMaterials(const aiScene* scene, D3D12ResourceBatch& batch,
            std::vector<Ref<HMaterial>>& materials);

        static void processNode(aiNode* node, const aiScene* scene,
            Ref<GameObject> target,
            D3D12ResourceBatch& batch,
            std::vector<Ref<HMaterial>>& materials);
    };
}


