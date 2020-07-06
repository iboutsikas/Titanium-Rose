#include <Hazel.h>
#include <Hazel/Core/EntryPoint.h>

//#include "ExampleLayer.h"
#include "BenchmarkLayer.h"

class Sandbox : public Hazel::Application
{
public:
	Sandbox(): Hazel::Application()
	{
		//PushLayer(new ExampleLayer());
	}

	virtual void OnInit() override 
	{
		PushLayer(new BenchmarkLayer());
	}

	~Sandbox()
	{
	}
};

Hazel::Application* Hazel::CreateApplication()
{
	return new Sandbox();
}
