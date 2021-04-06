#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

#include "BunnySceneLayer.h"
#include "SingleBunnyLayer.h"

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
			("u,update", "Update rate", cxxopts::value<int32_t>())
			("c,capture", "Frame capture rate", cxxopts::value<uint32_t>())
			("f,frames", "How many frames to capture", cxxopts::value<uint32_t>())
			("m,material", "Material to use in the test", cxxopts::value<std::string>())
			("e,experiment", "Title of the experiment to run", cxxopts::value<std::string>());

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

		if (options.count("update")) {
			opts.UpdateRate = options["update"].as<int32_t>();
		}

        if (options.count("capture")) {
			opts.CaptureRate = options["capture"].as<uint32_t>();
        }

        if (options.count("frames")) {
            opts.CaptureLimit = options["frames"].as<uint32_t>();
        }

        if (options.count("material")) {
			opts.MaterialName = options["material"].as<std::string>();
        }
		
        if (options.count("experiment")) {
			opts.ExperimentName = options["experiment"].as<std::string>();
        }
		

		//PushLayer(new BunnySceneLayer());
		PushLayer(new SingleBunnyLayer(opts));
	}

	~RoseGarden()
	{
	}
};

Hazel::Application* Hazel::CreateApplication()
{
	return new RoseGarden({ 1920, 1080, "Titanium Rose" });
}
