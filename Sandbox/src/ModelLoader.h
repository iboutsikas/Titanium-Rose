#pragma once

#include "Hazel/ComponentSystem/GameObject.h"

#include "Platform/D3D12/D3D12UploadBatch.h"

class ModelLoader
{
public:
	static Hazel::Ref<Hazel::GameObject>LoadFromFile(std::string& filepath, Hazel::D3D12ResourceUploadBatch& batch, bool swapHandeness = false);
};

