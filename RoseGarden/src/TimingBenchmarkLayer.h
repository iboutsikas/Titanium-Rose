#pragma once
#include "BenchmarkLayer.h"

#define DEFAULT_MATERIAL_NAME "StaticBlueChrome"

class TimingBenchmarkLayer : public BenchmarkLayer
{
public:
	struct CreationOptions: public BenchmarkLayer::CreationOptions {
		bool EnableDecoupled = false;		
		std::string ExperimentGroup = DEFAULT_MATERIAL_NAME;
		std::string ExperimentName = "capture0-update0";
		std::string Scene;
	};

	TimingBenchmarkLayer(CreationOptions& opts);

private:
	CreationOptions m_CreationOptions;

};

