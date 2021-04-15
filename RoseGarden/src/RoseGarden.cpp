#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include "BunnySceneLayer.h"
#include "SingleBunnyLayer.h"
#include "EmptyLayer.h"

class RoseGarden : public Hazel::Application
{
public:
	RoseGarden(const Hazel::Application::ApplicationOptions& opts): Hazel::Application(opts)
	{
		//PushLayer(new ExampleLayer());
	}

	virtual void AddApplicationOptions(cxxopts::Options& options) override 
	{
		options.add_options("Rose Garden")
			("p,patrol", "Enable patrol")
			("d,decoupled", "Enable decoupled rendering")
			("t,timing", "Whether the app should capture timing data")
			("u,update", "Update rate", cxxopts::value<int32_t>())
			("c,capture", "Frame capture rate", cxxopts::value<uint32_t>())
			("f,frames", "How many frames to capture", cxxopts::value<int32_t>())
			("g,group", "The experiment group", cxxopts::value<std::string>())
			("e,experiment", "Title of the experiment to run", cxxopts::value<std::string>())
			("s,scene", "The scene to load", cxxopts::value<std::string>())
			;


	}

	virtual void OnInit(cxxopts::ParseResult& options) override 
	{
		SingleBunnyLayer::CreationOptions opts;

		if (options.count("patrol")) {
			opts.EnablePatrol = true;
		}

        if (options.count("decoupled")) {
            opts.EnableDecoupled = true;
        }

		if (options.count("timing")) {
			opts.CaptureTiming = true;
		}

		if (options.count("update")) {
			opts.UpdateRate = options["update"].as<int32_t>();
		}

        if (options.count("capture")) {
			opts.CaptureRate = options["capture"].as<uint32_t>();
        }

        if (options.count("frames")) {
            opts.CaptureLimit = options["frames"].as<int32_t>();
        }

        if (options.count("group")) {
			opts.ExperimentGroup = options["group"].as<std::string>();
        }
		
        if (options.count("experiment")) {
			opts.ExperimentName = options["experiment"].as<std::string>();
        }
		
        if (options.count("scene")) {
            opts.Scene = options["scene"].as<std::string>();
        }

		//PushLayer(new BunnySceneLayer());
		PushLayer(new SingleBunnyLayer(opts));
		//PushLayer(new EmptyLayer());
	}

	~RoseGarden()
	{
	}
};

Hazel::Application* Hazel::CreateApplication()
{
	return new RoseGarden({ 1920, 1080, "Titanium Rose" });
}
