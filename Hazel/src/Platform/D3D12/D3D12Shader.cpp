#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Shader.h"
#include "d3dcompiler.h"
#include <sstream>

namespace Hazel {

	D3D12Shader::D3D12Shader(const std::string& filepath, CD3DX12_PIPELINE_STATE_STREAM pipelineStream, ShaderType shaderTypes)
		:m_Path(filepath), m_PipelineDesc(pipelineStream), m_ShaderTypes(shaderTypes)
	{
		HZ_PROFILE_FUNCTION();

		bool compilationResult = Recompile(&m_PipelineDesc);
		HZ_CORE_ASSERT(compilationResult, "Shader compilation failed");
		UpdateReferences();

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
	
	ID3D12RootSignature* D3D12Shader::GetRootSignature()
	{
		auto ret = m_RootSignature.Get();
		return ret;
	}

	ID3D12PipelineState* D3D12Shader::GetPipelineState()
	{
		auto ret = m_PipelineState.Get();
		return ret;
	}

	bool D3D12Shader::Recompile(void* stream)
	{
		CD3DX12_PIPELINE_STATE_STREAM* pipelineStream = static_cast<CD3DX12_PIPELINE_STATE_STREAM*>(stream);

		if ((m_ShaderTypes & ShaderType::Compute) == ShaderType::Compute)
		{
			return RecompileCompute(pipelineStream);
		}

		return RecompileGraphics(pipelineStream);
	}

	void D3D12Shader::UpdateReferences()
	{
		if (m_CompilationState != nullptr) {
			m_RootSignature = m_CompilationState->rootSignature;
			m_PipelineState = m_CompilationState->pipelineState;

			if ((m_ShaderTypes & ShaderType::Vertex) == ShaderType::Vertex) {
				m_VertexBlob = m_CompilationState->vertexBlob;
			}

			if ((m_ShaderTypes & ShaderType::Fragment) == ShaderType::Fragment) {
				m_FragmentBlob = m_CompilationState->fragmentBlob;
			}

			if ((m_ShaderTypes & ShaderType::Geometry) == ShaderType::Geometry) {
				m_GeometryBlob = m_CompilationState->geometryBlob;
			}
			m_CompilationState = nullptr;
		}
	}

	std::string D3D12Shader::ReadFile(const std::string& filepath)
	{
		return std::string();
	
	}
	
	bool D3D12Shader::RecompileCompute(CD3DX12_PIPELINE_STATE_STREAM* pipelineStream)
	{
		if (pipelineStream != nullptr)
		{
			// copy anything we need from the new stream. Currently we don't need anythign for compute
		}

		m_CompilationState.reset(new CompilationSate());

		m_Errors.clear();
		std::wstring stemp = std::wstring(m_Path.begin(), m_Path.end());

		if (FAILED(Compile(stemp, "CS_Main", "cs_5_1", &m_CompilationState->computeBlob))) {
			goto r_cleanup;
		}

		if (FAILED(ExtractRootSignature(m_CompilationState.get(), m_CompilationState->computeBlob))) {
			goto r_cleanup;
		}
		if (FAILED(BuildPSO(m_CompilationState.get(), &m_PipelineDesc))) {
			goto r_cleanup;
		}

		return true;

	r_cleanup:
		m_CompilationState.reset();
		m_CompilationState = nullptr;
		return false;
	}

	bool D3D12Shader::RecompileGraphics(CD3DX12_PIPELINE_STATE_STREAM* pipelineStream)
	{
		if (pipelineStream != nullptr) {
			m_PipelineDesc.InputLayout = pipelineStream->InputLayout;
			m_PipelineDesc.PrimitiveTopologyType = pipelineStream->PrimitiveTopologyType;
			m_PipelineDesc.DSVFormat = pipelineStream->DSVFormat;
			m_PipelineDesc.RTVFormats = pipelineStream->RTVFormats;
			m_PipelineDesc.RasterizerState = pipelineStream->RasterizerState;
		}

		m_CompilationState.reset(new CompilationSate());

		m_Errors.clear();
		std::wstring stemp = std::wstring(m_Path.begin(), m_Path.end());

		if (FAILED(Compile(stemp, "VS_Main", "vs_5_1", &m_CompilationState->vertexBlob))) {
			goto r_cleanup;
		}
		if (FAILED(Compile(stemp, "PS_Main", "ps_5_1", &m_CompilationState->fragmentBlob))) {
			goto r_cleanup;
		}

		if ((m_ShaderTypes & ShaderType::Geometry) == ShaderType::Geometry) {
			if (FAILED(Compile(stemp, "GS_Main", "gs_5_1", &m_CompilationState->geometryBlob))) {
				goto r_cleanup;
			}
		}

		if (FAILED(ExtractRootSignature(m_CompilationState.get(), m_CompilationState->vertexBlob))) {
			goto r_cleanup;
		}
		if (FAILED(BuildPSO(m_CompilationState.get(), &m_PipelineDesc))) {
			goto r_cleanup;
		}

		// Everything has compiled, time to swap them around. This only works in single thread mode
		return true;

	r_cleanup:
		m_CompilationState.reset();
		m_CompilationState = nullptr;
		return false;
	}

	HRESULT D3D12Shader::Compile(const std::wstring& filepathW, LPCSTR entryPoint, LPCSTR profile, ID3DBlob** blob)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( HZ_DEBUG)
		flags |= D3DCOMPILE_DEBUG;
		flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
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
				m_Errors.push_back((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			if (shaderBlob)
				shaderBlob->Release();

			return hr;
		}

		*blob = shaderBlob;

		return hr;
	}
	
	HRESULT D3D12Shader::ExtractRootSignature(CompilationSate* state, TComPtr<ID3DBlob> shaderBlob)
	{
		TComPtr<ID3DBlob> rootSignatureBlob;
		HRESULT hr;

		hr = D3DGetBlobPart(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), 
			D3D_BLOB_ROOT_SIGNATURE, 
			0, 
			&rootSignatureBlob);
		
		if (FAILED(hr))
		{
			std::stringstream ss;
			ss << "Failed to extract Root Signature from " << m_Name << '\n';
			m_Errors.push_back(ss.str());
			return hr;
		}
			
		hr = D3D12Renderer::Context->DeviceResources->Device->CreateRootSignature(
				0,
				rootSignatureBlob->GetBufferPointer(),
				rootSignatureBlob->GetBufferSize(),
				IID_PPV_ARGS(&state->rootSignature)
		);

		if (FAILED(hr))
		{
			std::stringstream ss;
			ss << "Failed to create Root Signature for " << m_Name << '\n';
			m_Errors.push_back(ss.str());
			return hr;
		}

		return S_OK;
		
	}


	HRESULT D3D12Shader::BuildPSO(CompilationSate* state, CD3DX12_PIPELINE_STATE_STREAM* pipelineStream)
	{
		pipelineStream->pRootSignature = state->rootSignature.Get();

		
		if ((m_ShaderTypes & ShaderType::Vertex) == ShaderType::Vertex) {
			pipelineStream->VS = CD3DX12_SHADER_BYTECODE(state->vertexBlob.Get());
		}

		if ((m_ShaderTypes & ShaderType::Fragment) == ShaderType::Fragment) {
			pipelineStream->PS = CD3DX12_SHADER_BYTECODE(state->fragmentBlob.Get());
		}

		if ((m_ShaderTypes & ShaderType::Geometry) == ShaderType::Geometry) {
			pipelineStream->GS = CD3DX12_SHADER_BYTECODE(state->geometryBlob.Get());
		}

		if ((m_ShaderTypes & ShaderType::Compute) == ShaderType::Compute) {
			pipelineStream->CS = CD3DX12_SHADER_BYTECODE(state->computeBlob.Get());
		}


		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
			sizeof(CD3DX12_PIPELINE_STATE_STREAM), pipelineStream
		};

		auto hr = D3D12Renderer::Context->DeviceResources->Device->CreatePipelineState(
			&pipelineStateStreamDesc,
			IID_PPV_ARGS(&state->pipelineState));

		if (FAILED(hr))
		{
			std::stringstream ss;
			ss << "Failed to create Pipeline State for " << m_Name << '\n';
			m_Errors.push_back(ss.str());
		}

		return hr;
	}
}
