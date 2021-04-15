#pragma once
#include "BenchmarkLayer.h"

#define DEFAULT_MATERIAL_NAME "StaticBlueChrome"

class SingleBunnyLayer : public BenchmarkLayer
{
public:
	struct CreationOptions: public BenchmarkLayer::CreationOptions {
		bool EnablePatrol = false;
		bool EnableDecoupled = false;		
		std::string ExperimentGroup = DEFAULT_MATERIAL_NAME;
		std::string ExperimentName = "capture0-update0";
		std::string Scene = "assets/models/mirror_like.fbx";
	};

	SingleBunnyLayer(CreationOptions& opts);

private:
	CreationOptions m_CreationOptions;

};

