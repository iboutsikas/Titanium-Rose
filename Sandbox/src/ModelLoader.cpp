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
	translation.Length();
	node->mTransformation.Decompose(scale, rotation, translation);

	target->Name = std::string(scene->mRootNode->mName.C_Str());
	target->Transform.SetPosition(translation.x, translation.y, translation.z);
	target->Transform.SetScale(scale.x, scale.y, scale.z);
	target->Transform.SetRotation(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z));

	// Load Mesh(es) for target

	if (node->mNumMeshes >= 1) {
		// We only support 1 mesh per node/go right now
		aiMesh* aimesh = scene->mMeshes[node->mMeshes[0]];
		auto len = aimesh->mMaxBitangent.Length();
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
				//auto length = glm::length(the_vertex.Tangent);

				//void* p = &length;
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
		
		hmesh->vertexBuffer = Hazel::CreateRef<Hazel::D3D12VertexBuffer>((float*)vertices.data(), vertices.size() * sizeof(Vertex));
		hmesh->vertexBuffer->GetResource()->SetName(L"Vertex buffer");
		
		hmesh->indexBuffer = Hazel::CreateRef<Hazel::D3D12IndexBuffer>(indices.data(), indices.size());
		hmesh->indexBuffer->GetResource()->SetName(L"Index buffer");

		hmesh->vertices = vertices;
		hmesh->indices = indices;

		target->Mesh = hmesh;

		if (aimesh->HasTangentsAndBitangents()) {
			target->Mesh->maxTangent.x = aimesh->mMaxTangent.x;
			target->Mesh->maxTangent.y = aimesh->mMaxTangent.y;
			target->Mesh->maxTangent.z = aimesh->mMaxTangent.z;

			target->Mesh->maxBitangent.x = aimesh->mMaxBitangent.x;
			target->Mesh->maxBitangent.y = aimesh->mMaxBitangent.y;
			target->Mesh->maxBitangent.z = aimesh->mMaxBitangent.z;
		}
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

Hazel::Ref<Hazel::GameObject> ModelLoader::LoadFromFile(std::string& filepath, bool swapHandedness)
{
	Hazel::Ref<Hazel::GameObject> rootGO = Hazel::CreateRef<Hazel::GameObject>();

	Assimp::Importer importer;

	uint32_t importFlags = 0;

	if (swapHandedness) {
		importFlags |= aiProcess_ConvertToLeftHanded;
	}
	//importFlags |= aiProcess_JoinIdenticalVertices;
	importFlags |= aiProcess_CalcTangentSpace;

	const aiScene* scene = importer.ReadFile(filepath, importFlags);
	
	if (scene == nullptr) {
		HZ_ERROR("Error loading model {} :\n {}", filepath, importer.GetErrorString());
		HZ_ASSERT(false, "Model loading failed");
		return nullptr;
	}
	
	if (scene->mRootNode->mNumMeshes == 0) {
		processNode(scene->mRootNode->mChildren[0], scene, rootGO);
	}
	else {
		processNode(scene->mRootNode, scene, rootGO);
	}


	return rootGO;
}
