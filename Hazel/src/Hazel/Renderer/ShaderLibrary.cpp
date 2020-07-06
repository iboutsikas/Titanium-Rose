#include "hzpch.h"
#include "Hazel/Renderer/ShaderLibrary.h"

namespace Hazel
{
	ShaderLibrary::ShaderLibrary(uint32_t frameLatency)
		: m_FrameLatency(frameLatency), m_FrameCount(0)
	{
	}

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		HZ_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		HZ_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	bool ShaderLibrary::Exists(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

	void ShaderLibrary::Update()
	{
		if (m_FrameCount >= m_FrameLatency) {
			for (auto& [key, shader] : m_Shaders) {
				shader->UpdateReferences();
			}
		}
		m_FrameCount++;
	}
}