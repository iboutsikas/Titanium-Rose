#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Shader.h"
#include "d3dcompiler.h"
#include <sstream>

namespace Hazel {

	D3D12Shader::D3D12Shader(const std::string& filepath, PipelineStateStream pipelineStream)
		:m_Path(filepath), m_PipelineDesc(pipelineStream)
	{
		HZ_PROFILE_FUNCTION();

		m_Context = static_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());

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
		PipelineStateStream* pipelineStream = (PipelineStateStream * )stream;
		if (pipelineStream != nullptr) {
			m_PipelineDesc.InputLayout = pipelineStream->InputLayout;
			m_PipelineDesc.PrimitiveTopologyType = pipelineStream->PrimitiveTopologyType;
			m_PipelineDesc.DSVFormat = pipelineStream->DSVFormat;
			m_PipelineDesc.RTVFormats = pipelineStream->RTVFormats;
			m_PipelineDesc.Rasterizer = pipelineStream->Rasterizer;
		}
		m_CompilationState = new CompilationSate;

		m_Errors.clear();
		std::wstring stemp = std::wstring(m_Path.begin(), m_Path.end());
		if (FAILED(Compile(stemp, "VS_Main", "vs_5_1", &m_CompilationState->vertexBlob))) {
			delete m_CompilationState;
			m_CompilationState = nullptr;
			return false;
		}
		if (FAILED(Compile(stemp, "PS_Main", "ps_5_1", &m_CompilationState->fragmentBlob))) {
			delete m_CompilationState;
			m_CompilationState = nullptr;
			return false;
		}
		if (FAILED(ExtractRootSignature(m_CompilationState, m_CompilationState->vertexBlob))) {
			delete m_CompilationState;
			m_CompilationState = nullptr;
			return false;
		}
		if (FAILED(BuildPSO(m_CompilationState, &m_PipelineDesc))) {
			delete m_CompilationState;
			m_CompilationState = nullptr;
			return false;
		}

		// Everything has compiled, time to swap them around. This only works in single thread mode
		return true;
	}

	void D3D12Shader::UpdateReferences()
	{
		if (m_CompilationState != nullptr) {
			m_VertexBlob = m_CompilationState->vertexBlob;
			m_FragmentBlob = m_CompilationState->fragmentBlob;
			m_RootSignature = m_CompilationState->rootSignature;
			m_PipelineState = m_CompilationState->pipelineState;
			m_CompilationState = nullptr;
		}
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
			
		hr = m_Context->DeviceResources->Device->CreateRootSignature(
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


	HRESULT D3D12Shader::BuildPSO(CompilationSate* state, PipelineStateStream* pipelineStream)
	{
		pipelineStream->pRootSignature = m_RootSignature.Get();
		pipelineStream->VS = CD3DX12_SHADER_BYTECODE(state->vertexBlob.Get());
		pipelineStream->PS = CD3DX12_SHADER_BYTECODE(state->fragmentBlob.Get());


		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
			sizeof(PipelineStateStream), pipelineStream
		};

		auto hr = m_Context->DeviceResources->Device->CreatePipelineState(
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
