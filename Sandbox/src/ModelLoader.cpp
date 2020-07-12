#include "ModelLoader.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/vector3.h"
#include "assimp/quaternion.h"
#include "assimp/Vertex.h"

#include "Hazel/Renderer/Vertex.h"

#include "Platform/D3D12/D3D12Texture.h"

#include <glm/gtc/type_ptr.hpp>

Hazel::TextureLibrary* ModelLoader::TextureLibrary = nullptr;

void processNode(aiNode* node, const aiScene* scene, 
	Hazel::Ref<Hazel::GameObject>& target, 
	Hazel::D3D12ResourceBatch& batch,
	std::vector<Hazel::Ref<Hazel::HMaterial>>& materials)
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

		Hazel::Ref<Hazel::HMesh> hmesh = Hazel::CreateRef<Hazel::HMesh>();

		std::vector<Hazel::Vertex> vertices;
		std::vector<uint32_t> indices;
		// Textures ?!

		for (size_t i = 0; i < aimesh->mNumVertices; i++)
		{
#pragma region Vertices
			Hazel::Vertex the_vertex;

			auto& vert = aimesh->mVertices[i];
			auto vx = vert.x;
			auto vy = vert.y;
			auto vz = vert.z;

			the_vertex.Position = glm::vec3(vx, vy, vz);

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
			aiFace face = aimesh->mFaces[f];

			for (size_t i = 0; i < face.mNumIndices; i++)
			{
				indices.push_back(face.mIndices[i]);
			}
#pragma endregion
		}
		
		hmesh->vertexBuffer = Hazel::CreateRef<Hazel::D3D12VertexBuffer>(batch, (float*)vertices.data(), vertices.size() * sizeof(Hazel::Vertex));
		hmesh->vertexBuffer->GetResource()->SetName(L"Vertex buffer");
		
		hmesh->indexBuffer = Hazel::CreateRef<Hazel::D3D12IndexBuffer>(batch, indices.data(), indices.size());
		hmesh->indexBuffer->GetResource()->SetName(L"Index buffer");

		hmesh->vertices = vertices;
		hmesh->indices = indices;

		target->Mesh = hmesh;
	}

	// Recursive call for children of target
	for (int c = 0; c < node->mNumChildren; c++)
	{
		auto child = node->mChildren[c];
		auto childGO = Hazel::CreateRef<Hazel::GameObject>();
		target->AddChild(childGO);

		processNode(child, scene, childGO, batch, materials);
	}
}

Hazel::Ref<Hazel::GameObject> ModelLoader::LoadFromFile(std::string& filepath, Hazel::D3D12ResourceBatch& batch, bool swapHandedness)
{
	Hazel::Ref<Hazel::GameObject> rootGO = Hazel::CreateRef<Hazel::GameObject>();

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
	std::vector<Hazel::Ref<Hazel::HMaterial>> materials;
	materials.resize(scene->mNumMaterials);
	// load Materials
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		materials[i] = Hazel::CreateRef<Hazel::HMaterial>();
		aiMaterial* aiMaterial = scene->mMaterials[i];

		aiString name;
		aiMaterial->Get(AI_MATKEY_NAME, name);
		HZ_INFO("Material: {}", name.C_Str());

		materials[i]->Name = std::string(name.C_Str());

		aiColor3D diffuseColor;
		aiColor3D emissiveColor;
		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
		if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
		{
			materials[i]->EmissiveColor = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, 1.0f);
		}

		float shininess;
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);

		float metalness;
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness);

		float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
		materials[i]->Specular = shininess;

		aiString texturefile;
		// Albedo
		Hazel::Ref<Hazel::D3D12Texture2D> albedoTexture;
		if (scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
			std::string filename(texturefile.C_Str());
			std::string filepath = "assets/textures/" + filename;

			HZ_INFO("\tUsing diffuse texture: {}", filename);
			std::wstring widepath(filepath.begin(), filepath.end());
			if (TextureLibrary->Exists(widepath))
			{
				albedoTexture = TextureLibrary->GetTexture(widepath);
			}
			else
			{
				// Load the texture, get it into the Library
				Hazel::D3D12Texture2D::TextureCreationOptions opts;
				opts.Flags = D3D12_RESOURCE_FLAG_NONE;
				opts.Path = widepath;
				albedoTexture = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
				TextureLibrary->AddTexture(albedoTexture);
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
		Hazel::Ref<Hazel::D3D12Texture2D> normalsTexture = nullptr;
		if (scene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS) > 0)
		{
			scene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &texturefile);
			std::string filename(texturefile.C_Str());
			std::string filepath = "assets/textures/" + filename;

			HZ_INFO("\tUsing diffuse texture: {}", filename);
			std::wstring widepath(filepath.begin(), filepath.end());
			if (TextureLibrary->Exists(widepath))
			{
				normalsTexture = TextureLibrary->GetTexture(widepath);
			}
			else
			{
				Hazel::D3D12Texture2D::TextureCreationOptions opts;
				opts.Flags = D3D12_RESOURCE_FLAG_NONE;
				opts.Path = widepath;
				normalsTexture = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
				TextureLibrary->AddTexture(normalsTexture);
			}
			materials[i]->HasNormalTexture = true;
		}
		else
		{
			HZ_WARN("\tNot using normal map");
			materials[i]->HasNormalTexture = false;
		}
		materials[i]->NormalTexture = normalsTexture;

		// Specular 
		Hazel::Ref<Hazel::D3D12Texture2D> specularTexture = nullptr;
		if (scene->mMaterials[i]->GetTextureCount(aiTextureType_SPECULAR) > 0)
		{
			scene->mMaterials[i]->GetTexture(aiTextureType_SPECULAR, 0, &texturefile);
			std::string filename(texturefile.C_Str());
			std::string filepath = "assets/textures/" + filename;
			HZ_INFO("\tUsing specular texture: {}", filename);

			std::wstring widepath(filepath.begin(), filepath.end());
			if (TextureLibrary->Exists(widepath))
			{
				specularTexture = TextureLibrary->GetTexture(widepath);
			}
			else
			{
				Hazel::D3D12Texture2D::TextureCreationOptions opts;
				opts.Flags = D3D12_RESOURCE_FLAG_NONE;
				opts.Path = widepath;
				specularTexture = Hazel::D3D12Texture2D::CreateCommittedTexture(batch, opts);
				TextureLibrary->AddTexture(specularTexture);
			}
			materials[i]->HasSpecularTexture = true;
		}
		else
		{
			HZ_WARN("\tNot using specular map");
			materials[i]->HasSpecularTexture = false;
		}
		materials[i]->SpecularTexture = specularTexture;

		// Alpha
		if (scene->mMaterials[i]->GetTextureCount(aiTextureType_OPACITY) > 0)
		{
			HZ_INFO("\tMaterial has transparency");
			materials[i]->IsTransparent = true;
		}
	}

	

	processNode(scene->mRootNode, scene, rootGO, batch, materials);



	return rootGO;
}
