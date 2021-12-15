#pragma once

#include "TitaniumRose/ComponentSystem/GameObject.h"
#include "TitaniumRose/Renderer/TextureLibrary.h"
#include "TitaniumRose/Scene/Scene.h"

//#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/CommandContext.h"


struct aiScene;
struct aiNode;

namespace Roses
{
    class ModelLoader
    {
    public:
        static Ref<HGameObject>LoadFromFile(std::string& filepath, bool swapHandeness = false);

        static void LoadScene(Scene& scene, std::string& filepath, bool swapHandeness = false);

    private:
        static void extractMaterials(const aiScene* scene, std::vector<Ref<HMaterial>>& materials);

        static void processNode(aiNode* node, const aiScene* scene,
            Ref<HGameObject> target,
            CommandContext& context,
            std::vector<Ref<HMaterial>>& materials);
    };
}


