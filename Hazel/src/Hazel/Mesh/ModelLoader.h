#pragma once

#include "Hazel/ComponentSystem/GameObject.h"
#include "Hazel/Renderer/TextureLibrary.h"
#include "Hazel/Scene/Scene.h"

//#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "Platform/D3D12/CommandContext.h"


struct aiScene;
struct aiNode;

namespace Hazel
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


