#pragma once

#include "Hazel/Renderer/Shader.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "d3d12.h"

namespace Hazel {
	class D3D12Shader : public Shader
	{
	public:
		D3D12Shader(const std::string& filepath);
		D3D12Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~D3D12Shader();

		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		virtual void SetInt(const std::string& name, int value) override {};
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override {};
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override {};
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override {};

		virtual const std::string& GetName() const override { return m_Name; }

		void UploadUniformInt(const std::string& name, int value) {};

		void UploadUniformFloat(const std::string& name, float value) {};
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value) {};
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value) {};
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value) {};

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix) {};
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix) {};

		inline TComPtr<ID3DBlob> GetVertexBlob() const { return m_VertexBlob; }
		inline TComPtr<ID3DBlob> GetFragmentBlob() const { return m_FragmentBlob; }
	private:
		std::string ReadFile(const std::string& filepath);
		
		HRESULT Compile(const std::wstring& filepathW, LPCSTR entryPoint, LPCSTR profile, ID3DBlob** blob);
	private:
		TComPtr<ID3DBlob> m_VertexBlob;
		TComPtr<ID3DBlob> m_FragmentBlob;
		D3D12Context* m_Context;
		std::string m_Name;
	};
}
