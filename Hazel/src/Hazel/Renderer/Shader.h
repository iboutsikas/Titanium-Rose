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
		
		virtual void UpdateReferences() = 0;

		virtual bool Recompile(void* pipelineStream = nullptr) { return true; }
		virtual std::vector<std::string>& GetErrors() { return m_Errors; }
		virtual std::vector<std::string>& GetWarnings() { return m_Warnings; }

		virtual const std::string& GetName() const { return m_Name; }
	protected:
		std::vector<std::string> m_Errors;
		std::vector<std::string> m_Warnings;
		std::string m_Name;
	};
}