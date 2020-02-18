#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Shader.h"
#include "d3dcompiler.h"

namespace Hazel {

	D3D12Shader::D3D12Shader(const std::string& filepath)
	{
		HZ_PROFILE_FUNCTION();

		m_Context = static_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());

		std::wstring stemp = std::wstring(filepath.begin(), filepath.end());
		
		D3D12::ThrowIfFailed(Compile(stemp, "VS_Main", "vs_5_1", &m_VertexBlob));
		D3D12::ThrowIfFailed(Compile(stemp, "PS_Main", "ps_5_1", &m_FragmentBlob));

		// Extract name from filepath
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	D3D12Shader::D3D12Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) {

	}

	D3D12Shader::~D3D12Shader()
	{
	}
	
	std::string D3D12Shader::ReadFile(const std::string& filepath)
	{
		return std::string();
	
	}
	HRESULT D3D12Shader::Compile(const std::wstring& filepathW, LPCSTR entryPoint, LPCSTR profile, ID3DBlob** blob)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( HZ_DEBUG)
		flags |= D3DCOMPILE_DEBUG;
#endif
		ID3DBlob* shaderBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;

		HRESULT hr = D3DCompileFromFile(
			filepathW.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint,
			profile,
			flags, 0, 
			&shaderBlob, 
			&errorBlob);

		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				HZ_CORE_ERROR("Error compiling shader: {}", (char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			if (shaderBlob)
				shaderBlob->Release();

			return hr;
		}

		*blob = shaderBlob;

		return hr;
	}
}
