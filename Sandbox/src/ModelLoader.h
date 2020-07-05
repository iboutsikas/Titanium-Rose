#pragma once

#include "Hazel/ComponentSystem/GameObject.h"

#include "Platform/D3D12/D3D12ResourceBatch.h"
#include "TextureLibrary.h"

class ModelLoader
{
public:
	static TextureLibrary* TextureLibrary;
	static Hazel::Ref<Hazel::GameObject>LoadFromFile(std::string& filepath, Hazel::D3D12ResourceBatch& batch, bool swapHandeness = false);
};

