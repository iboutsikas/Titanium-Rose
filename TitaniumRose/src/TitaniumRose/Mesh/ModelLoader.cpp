#include "trpch.h"

#include "TitaniumRose/Mesh/ModelLoader.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/vector3.h"
#include "assimp/quaternion.h"
#include "assimp/Vertex.h"

#include "TitaniumRose/Renderer/Vertex.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Texture.h"
//#undef min
//#undef max
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Roses
{

    Ref<HGameObject> ModelLoader::LoadFromFile(std::string& filepath, bool swapHandedness)
    {
        Ref<HGameObject> rootGO = CreateRef<HGameObject>();

        Assimp::Importer importer;

        uint32_t importFlags =
            //aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
            //aiProcess_Triangulate |             // Make sure we're triangles
            //aiProcess_SortByPType |             // Split meshes by primitive type
            //aiProcess_GenNormals |              // Make sure we have legit normals
            //aiProcess_GenUVCoords |             // Convert UVs if required 
            //aiProcess_OptimizeMeshes |          // Batch draws where possible
            //aiProcess_ValidateDataStructure;    // Validation
            aiProcess_Triangulate |
            aiProcess_CalcTangentSpace |
            aiProcess_GenUVCoords |
            aiProcess_GenNormals |
            aiProcess_TransformUVCoords |
            aiProcess_ValidateDataStructure;

        if (swapHandedness) {
            importFlags |= aiProcess_ConvertToLeftHanded;
        }

        const aiScene* scene = importer.ReadFile(filepath, importFlags);

        if (scene == nullptr) {
            HZ_ERROR("Error loading model {} :\n {}", filepath, importer.GetErrorString());
            HZ_ASSERT(false, "Model loading failed");
            return nullptr;
        }

        std::vector<Ref<HMaterial>> materials(scene->mNumMaterials);

        extractMaterials(scene,materials);

        aiNode* node = scene->mRootNode->mNumMeshes == 0 ? scene->mRootNode->mChildren[0] : scene->mRootNode;
        
        GraphicsContext& context = GraphicsContext::Begin();
        ModelLoader::processNode(node, scene, rootGO, context, materials);
        context.Finish(true);
        return rootGO;
    }

    void ModelLoader::LoadScene(Scene& scene, std::string& filepath, bool swapHandeness)
    {
        Assimp::Importer importer;

        uint32_t importFlags =
            aiProcess_Triangulate |
            aiProcess_CalcTangentSpace |
            aiProcess_GenUVCoords |
            aiProcess_GenNormals |
            aiProcess_TransformUVCoords |
            aiProcess_ValidateDataStructure;

        if (swapHandeness) {
            importFlags |= aiProcess_ConvertToLeftHanded;
        }

        const aiScene* aScene = importer.ReadFile(filepath, importFlags);

        if (aScene == nullptr) {
            HZ_ERROR("Error loading model {} :\n {}", filepath, importer.GetErrorString());
            HZ_ASSERT(false, "Model loading failed");
        }
        std::vector<Ref<HMaterial>> materials(aScene->mNumMaterials);


        extractMaterials(aScene, materials);
        GraphicsContext& context = GraphicsContext::Begin();
        for (int c = 0; c < aScene->mRootNode->mNumChildren; c++)
        {
            auto child = aScene->mRootNode->mChildren[c];
            auto childGO = CreateRef<HGameObject>();
            scene.AddEntity(childGO);

            processNode(child, aScene, childGO, context, materials);
        }
        context.Finish(true);
    }

    void ModelLoader::processNode(aiNode* node, const aiScene* scene,
        Ref<HGameObject> target,
        CommandContext& context,
        std::vector<Ref<HMaterial>>& materials)
    {
        aiVector3D translation;
        aiVector3D scale;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scale, rotation, translation);

        target->Name = std::string(node->mName.C_Str());
        target->Transform.SetPosition(translation.x, translation.y, translation.z);
        target->Transform.SetScale(scale.x, scale.y, scale.z);
        target->Transform.SetRotation(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z));

        // Load Mesh(es) for target

        if (node->mNumMeshes > 0) {
            // We only support 1 mesh per node/go right now
            aiMesh* aimesh = scene->mMeshes[node->mMeshes[0]];
            target->Material = materials[aimesh->mMaterialIndex];

            Ref<HMesh> hmesh = CreateRef<HMesh>();

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            AABB boundingBox({ FLT_MAX, FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX });

            for (size_t i = 0; i < aimesh->mNumVertices; i++)
            {
#pragma region Vertices
                Vertex the_vertex;

                auto& vert = aimesh->mVertices[i];
                auto vx = vert.x;
                auto vy = vert.y;
                auto vz = vert.z;

                the_vertex.Position = glm::vec3(vx, vy, vz);

                boundingBox.Min.x = glm::min(vx, boundingBox.Min.x);
                boundingBox.Min.y = glm::min(vy, boundingBox.Min.y);
                boundingBox.Min.z = glm::min(vz, boundingBox.Min.z);
                boundingBox.Max.x = glm::max(vx, boundingBox.Max.x);
                boundingBox.Max.y = glm::max(vy, boundingBox.Max.y);
                boundingBox.Max.z = glm::max(vz, boundingBox.Max.z);

                if (aimesh->HasNormals())
                {
                    float nx = aimesh->mNormals[i].x;
                    float ny = aimesh->mNormals[i].y;
                    float nz = aimesh->mNormals[i].z;
                    the_vertex.Normal = glm::vec3(nx, ny, nz);
                }

                if (aimesh->HasTangentsAndBitangents())
                {
                    auto tx = aimesh->mTangents[i].x;
                    auto ty = aimesh->mTangents[i].y;
                    auto tz = aimesh->mTangents[i].z;
                    the_vertex.Tangent = glm::vec3(tx, ty, tz);

                    the_vertex.Binormal = glm::make_vec3((float*)&aimesh->mBitangents[i].x);
                }

                if (aimesh->HasTextureCoords(0))
                {
                    auto tu = aimesh->mTextureCoords[0][i].x;
                    auto tv = aimesh->mTextureCoords[0][i].y;
                    the_vertex.UV = glm::vec2(tu, tv);
                }

                vertices.push_back(the_vertex);
#pragma endregion
            }

            for (size_t f = 0; f < aimesh->mNumFaces; f++)
#pragma region Indices
            {
                aiFace& face = aimesh->mFaces[f];
                HZ_CORE_ASSERT(face.mNumIndices == 3, "Can only deal with triangles right now!");
                Triangle tri;
                tri.V0 = &vertices[face.mIndices[0]];
                indices.push_back(face.mIndices[0]);
                tri.V1 = &vertices[face.mIndices[1]];
                indices.push_back(face.mIndices[1]);
                tri.V2 = &vertices[face.mIndices[2]];
                indices.push_back(face.mIndices[2]);
                hmesh->triangles.push_back(tri);
#pragma endregion
            }

            hmesh->vertexBuffer = CreateRef<D3D12VertexBuffer>(context, (float*)vertices.data(), vertices.size() * sizeof(Vertex));
            hmesh->vertexBuffer->GetResource()->SetName(L"Vertex buffer");

            hmesh->indexBuffer = CreateRef<D3D12IndexBuffer>(context, indices.data(), indices.size());
            hmesh->indexBuffer->GetResource()->SetName(L"Index buffer");

            hmesh->vertices.swap(vertices);
            hmesh->indices.swap(indices);
            hmesh->BoundingBox = boundingBox;
            target->Mesh = hmesh;
        }

        // Recursive call for children of target
        for (int c = 0; c < node->mNumChildren; c++)
        {
            auto child = node->mChildren[c];
            auto childGO = CreateRef<HGameObject>();
            target->AddChild(childGO);

            processNode(child, scene, childGO, context, materials);
        }
    }

    void ModelLoader::extractMaterials(const aiScene* scene, std::vector<Ref<HMaterial>>& materials)
    {
        for (size_t i = 0; i < scene->mNumMaterials; i++)
        {
            materials[i] = CreateRef<HMaterial>();
            aiMaterial* aiMaterial = scene->mMaterials[i];

            aiString name;
            aiMaterial->Get(AI_MATKEY_NAME, name);
            HZ_INFO("Material: {}", name.C_Str());

            materials[i]->Name = std::string(name.C_Str());

            aiColor3D diffuseColor = { 1.0f, 1.0f, 1.0f };
            if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
            {
                materials[i]->Color = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
            }

            aiColor3D emissiveColor = { 0.0f, 0.0f, 0.0f };
            if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
            {
                materials[i]->EmissiveColor = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, 1.0f);
            }

            float shininess = 1.0f;
            aiMaterial->Get(AI_MATKEY_SHININESS, shininess);

            float metalness = 0.0f;
            aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness);

            float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
            aiString texturefile;
            // Albedo
            Ref<Texture2D> albedoTexture;
            if (scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
                std::string filename(texturefile.C_Str());
                std::string filepath = "assets/textures/" + filename;

                HZ_INFO("\tUsing diffuse texture: {}", filename);
                if (D3D12Renderer::g_TextureLibrary->Exists(filepath))
                {
                    albedoTexture = D3D12Renderer::g_TextureLibrary->GetAs<Texture2D>(filepath);
                }
                else
                {
                    // Load the texture, get it into the Library
                    TextureCreationOptions opts;
                    opts.Flags = D3D12_RESOURCE_FLAG_NONE;
                    opts.Path = filepath;
                    albedoTexture = Texture2D::CreateCommittedTexture(opts);
                    D3D12Renderer::g_TextureLibrary->Add(albedoTexture);
                }
                materials[i]->HasAlbedoTexture = true;
                materials[i]->Color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
            else
            {
                // Set this to some dummy texture
                HZ_WARN("\tNot using diffuse texture");
                materials[i]->HasAlbedoTexture = false;
                materials[i]->Color = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f);
            }
            // Set the texture for the material
            materials[i]->AlbedoTexture = albedoTexture;

            // Normals
            Ref<Texture2D> normalsTexture = nullptr;
            if (scene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS) > 0)
            {
                scene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &texturefile);
                std::string filename(texturefile.C_Str());
                std::string filepath = "assets/textures/" + filename;

                HZ_INFO("\tUsing diffuse texture: {}", filename);
                if (D3D12Renderer::g_TextureLibrary->Exists(filepath))
                {
                    normalsTexture = D3D12Renderer::g_TextureLibrary->GetAs<Texture2D>(filepath);
                }
                else
                {
                    TextureCreationOptions opts;
                    opts.Flags = D3D12_RESOURCE_FLAG_NONE;
                    opts.Path = filepath;
                    normalsTexture = Texture2D::CreateCommittedTexture(opts);
                    D3D12Renderer::g_TextureLibrary->Add(normalsTexture);
                }
                materials[i]->HasNormalTexture = true;
            }
            else
            {
                HZ_WARN("\tNot using normal map");
                materials[i]->HasNormalTexture = false;
            }
            materials[i]->NormalTexture = normalsTexture;

            // Roughness
            Ref<Texture2D> roughnessTexture = nullptr;
            if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &texturefile) == AI_SUCCESS)
            {
                std::string filename(texturefile.C_Str());
                std::string filepath = "assets/textures/" + filename;

                HZ_INFO("\tUsing roughness texture: {}", filename);

                if (D3D12Renderer::g_TextureLibrary->Exists(filepath))
                {
                    roughnessTexture = D3D12Renderer::g_TextureLibrary->GetAs<Texture2D>(filepath);
                }
                else
                {
                    TextureCreationOptions opts;
                    opts.Flags = D3D12_RESOURCE_FLAG_NONE;
                    opts.Path = filepath;
                    roughnessTexture = Texture2D::CreateCommittedTexture(opts);
                    D3D12Renderer::g_TextureLibrary->Add(roughnessTexture);
                }
                materials[i]->HasRoughnessTexture = true;
            }
            else
            {
                HZ_WARN("\tNot using roughness map");
                materials[i]->HasRoughnessTexture = false;
                materials[i]->Roughness = roughness;
            }
            materials[i]->RoughnessTexture = roughnessTexture;

            // Metallic
            bool metallicFound = false;
            std::string filepath;
            if (aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &texturefile) == AI_SUCCESS)
            {
                std::string filename(texturefile.C_Str());
                filepath = "assets/textures/" + filename;

                HZ_INFO("\tUsing metallic texture: {}", filename);
                metallicFound = true;

            }
            else
            {
                for (uint32_t p = 0; p < aiMaterial->mNumProperties; p++)
                {
                    auto prop = aiMaterial->mProperties[p];
                    std::string key = prop->mKey.data;
#if 0
                    HZ_TRACE("\t\tProperty: {}", key);
#endif
                    if (prop->mType == aiPTI_String)
                    {
                        uint32_t length = *(uint32_t*)prop->mData;
                        std::string str(prop->mData + 4, length);

                        // The raw property for reflection, ie Metalness. Since 
                        // it is not mapped to the texture earlier for some reason
                        if (key == "$raw.ReflectionFactor|file")
                        {
                            filepath = "assets/textures/" + str;
                            metallicFound = true;
                            break;
                        }
                    }
                }
            }

            if (metallicFound)
            {
                Ref<Texture2D> metallicTexture = nullptr;
                HZ_INFO("\tUsing metallic texture: {}", filepath);

                if (D3D12Renderer::g_TextureLibrary->Exists(filepath))
                {
                    metallicTexture = D3D12Renderer::g_TextureLibrary->GetAs<Texture2D>(filepath);
                }
                else
                {
                    TextureCreationOptions opts;
                    opts.Flags = D3D12_RESOURCE_FLAG_NONE;
                    opts.Path = filepath;
                    metallicTexture = Texture2D::CreateCommittedTexture(opts);
                    D3D12Renderer::g_TextureLibrary->Add(metallicTexture);
                }
                materials[i]->HasMetallicTexture = true;
                materials[i]->MetallicTexture = metallicTexture;
            }
            else
            {
                HZ_WARN("\tNot using roughness map");
                materials[i]->HasMetallicTexture = false;
                materials[i]->Metallic = metalness;
            }


            // Alpha
            if (scene->mMaterials[i]->GetTextureCount(aiTextureType_OPACITY) > 0)
            {
                HZ_INFO("\tMaterial has transparency");
                materials[i]->IsTransparent = true;
            }
        }
    }

}






