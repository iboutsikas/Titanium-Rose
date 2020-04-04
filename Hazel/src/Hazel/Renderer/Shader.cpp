#include "hzpch.h"
#include "Hazel/Renderer/Shader.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/D3D12/D3D12Shader.h"

namespace Hazel {

	Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
			//case RendererAPI::API::D3D12:	return CreateRef<D3D12Shader>(filepath);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	ShaderLibrary* ShaderLibrary::s_GlobalLibrary = nullptr;

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

	ShaderLibrary* ShaderLibrary::GlobalLibrary()
	{
		return s_GlobalLibrary;
	}

	void ShaderLibrary::InitalizeGlobalLibrary(uint32_t frameLatency)
	{
		s_GlobalLibrary = new ShaderLibrary(frameLatency);
	}

}