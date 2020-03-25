#include "ModelLoader.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/vector3.h"
#include "assimp/quaternion.h"
#include "assimp/Vertex.h"
#include "Vertex.h"

void processNode(aiNode* node, const aiScene* scene, Hazel::Ref<Hazel::GameObject>& target)
{
	aiVector3D translation;
	aiVector3D scale;
	aiQuaternion rotation;

	node->mTransformation.Decompose(scale, rotation, translation);

	target->Name = std::string(scene->mRootNode->mName.C_Str());
	target->Transform.SetPosition(translation.x, translation.y, translation.z);
	target->Transform.SetScale(scale.x, scale.y, scale.z);
	target->Transform.SetRotation(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z));

	// Load Mesh(es) for target

	if (node->mNumMeshes >= 1) {
		// We only support 1 mesh per node/go right now
		aiMesh* aimesh = scene->mMeshes[node->mMeshes[0]];
		Hazel::Ref<Hazel::HMesh> hmesh = Hazel::CreateRef<Hazel::HMesh>();

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		// Textures ?!

		for (size_t i = 0; i < aimesh->mNumVertices; i++)
		{
#pragma region Vertices
			Vertex the_vertex;

			auto& vert = aimesh->mVertices[i];
			auto vx = vert.x;
			auto vy = vert.y;
			auto vz = vert.z;

			the_vertex.Position = glm::vec3(vx, vy, vz);

			if (aimesh->HasNormals())
			{
				auto nx = aimesh->mNormals[i].x;
				auto ny = aimesh->mNormals[i].y;
				auto nz = aimesh->mNormals[i].z;
				the_vertex.Normal = glm::vec3(vx, vy, vz);
			}
			else
			{
				HZ_WARN("Model has no normals. WTF did you load?");
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				auto tx = aimesh->mTangents[i].x;
				auto ty = aimesh->mTangents[i].y;
				auto tz = aimesh->mTangents[i].z;
				the_vertex.Tangent = glm::vec3(tx, ty, tz);
			}
			else
			{
				HZ_INFO("No tangents in the mesh");
			}

			if (aimesh->HasTextureCoords(0))
			{
				auto tu = aimesh->mTextureCoords[0][i].x;
				auto tv = aimesh->mTextureCoords[0][i].y;
				the_vertex.UV = glm::vec2(tu, tv);
			}
			else
			{
				HZ_WARN("Model has no UVs. WTF did you load?");
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

		// TODO: Material
		
		hmesh->vertexBuffer = Hazel::CreateRef<Hazel::D3D12VertexBuffer>((float*)vertices.data(), vertices.size() * sizeof(Vertex));
		hmesh->vertexBuffer->GetResource()->SetName(L"Vertex buffer");
		
		hmesh->indexBuffer = Hazel::CreateRef<Hazel::D3D12IndexBuffer>(indices.data(), indices.size());
		hmesh->indexBuffer->GetResource()->SetName(L"Index buffer");
		
		target->Mesh = hmesh;
	}

	// Recursive call for children of target
	for (int c = 0; c < node->mNumChildren; c++)
	{
		auto child = node->mChildren[c];
		auto childGO = Hazel::CreateRef<Hazel::GameObject>();
		target->AddChild(childGO);

		processNode(child, scene, childGO);
	}
}

Hazel::Ref<Hazel::GameObject> ModelLoader::LoadFromFile(std::string& filepath)
{
	Hazel::Ref<Hazel::GameObject> rootGO = Hazel::CreateRef<Hazel::GameObject>();

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filepath, aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
	
	if (scene == nullptr) {
		HZ_ERROR("Error loading model {} :\n {}", filepath, importer.GetErrorString());
		HZ_ASSERT(false, "Model loading failed");
		return nullptr;
	}
	
	// TODO: Chances are we will end of with a ton of root objects named ROOT. Maybe fix that ?
	processNode(scene->mRootNode, scene, rootGO);

	return rootGO;
}
