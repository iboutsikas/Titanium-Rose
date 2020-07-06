#pragma once

#include <string>
#include <unordered_map>
#include <type_traits>

#include <glm/glm.hpp>
#include "Hazel/Core/BitfieldEnum.h"

namespace Hazel {

	enum class ShaderType {
		None = 0x00000000,
		Geometry = 0x00000001,
		Vertex = 0x00000010,
		Fragment = 0x00000100,
		Compute = 0x00001000	
	};
	ENABLE_BITMASK_OPERATORS(ShaderType);

	static constexpr ShaderType VertexAndFragment = ShaderType::Vertex | ShaderType::Fragment;

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

		virtual const std::string& GetName() const = 0;
		
		virtual void UpdateReferences() = 0;

		virtual bool Recompile(void* pipelineStream = nullptr) { return true; }
		virtual std::vector<std::string>& GetErrors() { return m_Errors; }

		static Ref<Shader> Create(const std::string& filepath);
		static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);

	protected:
		std::vector<std::string> m_Errors;
	};
}