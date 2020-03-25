#pragma once

#include "Hazel/ComponentSystem/GameObject.h"

class ModelLoader
{
public:
	static Hazel::Ref<Hazel::GameObject>LoadFromFile(std::string& filepath);
};

