#include <Roses.h>
#include <TitaniumRose/Core/EntryPoint.h>

#include "QualityBenchmarkLayer.h"
#include "TimingBenchmarkLayer.h"

class RoseGarden : public Roses::Application
{
public:
	RoseGarden(const Roses::Application::ApplicationOptions& opts): Roses::Application(opts)
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
		if (options.count("timing")) {
            m_UseFixedTimestep = false;

			TimingBenchmarkLayer::CreationOptions opts;
            opts.CaptureTiming = true;

            if (options.count("decoupled")) {
                opts.EnableDecoupled = true;
            }

            if (options.count("update")) {
                opts.UpdateRate = options["update"].as<int32_t>();
            }

            if (options.count("frames")) {
                opts.CaptureLimit = options["frames"].as<int32_t>();
            }
            else {
                // We are probably debugging so we do not want to terminate
                opts.CaptureTiming = false;
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

            /*opts.EnableDecoupled = true;
            opts.UpdateRate = 3;*/

            PushLayer(new TimingBenchmarkLayer(opts));
		}
		else {
            m_UseFixedTimestep = true;

            QualityBenchmarkLayer::CreationOptions opts;
            opts.CaptureTiming = false;

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

            PushLayer(new QualityBenchmarkLayer(opts));

		}
		//PushLayer(new EmptyLayer());
	}

	~RoseGarden()
    {
	}
};

Roses::Application* Roses::CreateApplication()
{
	return new RoseGarden({ 1920, 1080, "Titanium Rose" });
}
