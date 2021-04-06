#pragma once
#include "Hazel/Core/Core.h"
#include "cxxopts/include/cxxopts.hpp"

#include <unordered_map>
#ifdef HZ_PLATFORM_WINDOWS

extern Hazel::Application* Hazel::CreateApplication();

int main(int argc, char** argv)
{
	Hazel::Log::Init();

	cxxopts::Options options("RoseGarden", "My little rendering engine");

	options.allow_unrecognised_options();

	auto app = Hazel::CreateApplication();

	app->AddApplicationOptions(options);

	auto result = options.parse(argc, argv);

	app->Init();
	app->OnInit(result);
	app->Run();

	delete app;
}

#endif
