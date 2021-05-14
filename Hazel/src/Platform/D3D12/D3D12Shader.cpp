#include "hzpch.h"
#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Shader.h"

#include <dxcapi.h>
#include <d3d12shader.h>

#include <sstream>


enum ShaderProfile {
	GEOMETRY,
	VERTEX,
	PIXEL,
	COMPUTE,
	COUNT
};


// This matches ShaderType 1-1 for now
constexpr LPCWSTR TARGET_PROFILES[] = {
	L"gs_6_0",
	L"vs_6_0",
	L"ps_6_0",
	L"cs_6_0"
};



namespace Hazel {

	D3D12Shader::D3D12Shader(const std::string& filepath, CD3DX12_PIPELINE_STATE_STREAM pipelineStream, ShaderType shaderTypes)
		:m_Path(filepath), m_PipelineDesc(pipelineStream), m_ShaderTypes(shaderTypes)
	{
		HZ_PROFILE_FUNCTION();

        // Extract name from filepath
        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);

		bool compilationResult = Recompile(&m_PipelineDesc);
		HZ_CORE_ASSERT(compilationResult, "Shader compilation failed");
		UpdateReferences();
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
			
			HRESULT hr;
			if ((m_ShaderTypes & ShaderType::Vertex) == ShaderType::Vertex) {
				hr = m_CompilationState->vertexBlob->QueryInterface(IID_PPV_ARGS(&m_VertexBlob));

				if (FAILED(hr)) {
					HZ_CORE_ERROR("Failed to convert vertex shader during recompilation");
				}
			}

			if ((m_ShaderTypes & ShaderType::Fragment) == ShaderType::Fragment) {
                hr = m_CompilationState->fragmentBlob->QueryInterface(IID_PPV_ARGS(&m_FragmentBlob));

                if (FAILED(hr)) {
                    HZ_CORE_ERROR("Failed to convert fragment shader during recompilation");
                }
			}

			if ((m_ShaderTypes & ShaderType::Geometry) == ShaderType::Geometry) {
                hr = m_CompilationState->geometryBlob->QueryInterface(IID_PPV_ARGS(&m_GeometryBlob));

                if (FAILED(hr)) {
                    HZ_CORE_ERROR("Failed to convert geometry shader during recompilation");
                }
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
			// copy anything we need from the new stream. Currently we don't need anything for compute
		}

		m_CompilationState.reset(new CompilationSate());

		m_Errors.clear();
		std::wstring stemp = std::wstring(m_Path.begin(), m_Path.end());

		if (FAILED(Compile(stemp, L"CS_Main", TARGET_PROFILES[ShaderProfile::COMPUTE], &m_CompilationState->computeBlob))) {
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
		m_Warnings.clear();
		std::wstring stemp = std::wstring(m_Path.begin(), m_Path.end());

		if (FAILED(Compile(stemp, L"VS_Main", TARGET_PROFILES[ShaderProfile::VERTEX], &m_CompilationState->vertexBlob))) {
			goto r_cleanup;
		}
		if (FAILED(Compile(stemp, L"PS_Main", TARGET_PROFILES[ShaderProfile::PIXEL], &m_CompilationState->fragmentBlob))) {
			goto r_cleanup;
		}

		if ((m_ShaderTypes & ShaderType::Geometry) == ShaderType::Geometry) {
			if (FAILED(Compile(stemp, L"GS_Main", TARGET_PROFILES[ShaderProfile::GEOMETRY], &m_CompilationState->geometryBlob))) {
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

	HRESULT D3D12Shader::Compile(const std::wstring& filepathW, LPCWSTR entryPoint, LPCWSTR profile, IDxcBlob** blob)
	{
		static TComPtr<IDxcUtils> utils = nullptr;
		static TComPtr<IDxcCompiler3> compiler = nullptr;
		static TComPtr<IDxcIncludeHandler> includeHandler = nullptr;

		if (!utils) DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (!compiler) DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (!includeHandler) utils->CreateDefaultIncludeHandler(&includeHandler);

		
		std::wstring pdbName(m_Name.begin(), m_Name.end());
		pdbName.append(L".pdb");

		std::vector<LPCWSTR> args = {
			filepathW.c_str(),
			L"-E", entryPoint,
			L"-T", profile,
			L"-Zi",
			L"-Fd", pdbName.c_str(),
			L"-Od"
		};

		TComPtr<IDxcBlobEncoding> source = nullptr;
		D3D12::ThrowIfFailed(utils->LoadFile(filepathW.c_str(), nullptr, &source));
		
		DxcBuffer buffer;
		buffer.Ptr = source->GetBufferPointer();
		buffer.Size = source->GetBufferSize();
        buffer.Encoding = DXC_CP_ACP;
        //buffer.Encoding = DXC_CP_UTF8;


		TComPtr<IDxcResult> results;
		D3D12::ThrowIfFailed(compiler->Compile(
			&buffer,
			args.data(),
			args.size(),
			includeHandler.Get(),
			IID_PPV_ARGS(&results)
		));

		/*
		* We are always grabbing the errors first anyway, as warnings will be in this blob as well.
		* We want to print warnings, even if they didn't break compilation.
		*/
        TComPtr<IDxcBlobUtf8> errors = nullptr;
        results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		

		HRESULT compilationStatus;
		results->GetStatus(&compilationStatus);

		if (FAILED(compilationStatus)) {
			HZ_CORE_ASSERT(errors != nullptr, "errors should not be null, as compilation failed");
			HZ_CORE_ASSERT(errors->GetStringLength() != 0, "String length should not be 0 as compilation failed");
			// Anything in errors is considered an error now
			HZ_CORE_ERROR("Error compiling shader: {}", errors->GetStringPointer());
			m_Errors.push_back(errors->GetStringPointer());
			return compilationStatus;
		}
		else {
			// Anything in errors is considered a warning now, as compilation succeeded.
			if (errors->GetStringLength() != 0) {
				m_Warnings.push_back(errors->GetStringPointer());
				HZ_CORE_WARN("Warnings compiling shader: {}", errors->GetStringPointer());
			}
			results->GetResult(blob);

			TComPtr<IDxcBlob> pdb = nullptr;
			TComPtr<IDxcBlobUtf16> pdbName = nullptr;

			results->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &pdbName);
			std::wstring pdbFilePath = L"assets/shaders/";
			pdbFilePath.append(pdbName->GetStringPointer());
			// TODO: Get rid of C api
			FILE* fp = NULL;
			_wfopen_s(&fp, pdbFilePath.c_str(), L"wb");
			fwrite(pdb->GetBufferPointer(), pdb->GetBufferSize(), 1, fp);
			fclose(fp);
		}

		return compilationStatus;
#if 0
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

		if (errorBlob != nullptr) 
		{
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            HZ_CORE_WARN("Error compiling shader: {}", (char*)errorBlob->GetBufferPointer());
            m_Warnings.push_back((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
		}

		*blob = shaderBlob;

		return hr;
#endif
	}
	
	HRESULT D3D12Shader::ExtractRootSignature(CompilationSate* state, TComPtr<IDxcBlob> shaderBlob)
	{

		// With the new compiler, we do not need to extract anything. We can pass the same blob into
		// CreateRootSignature.
        HRESULT hr;
        hr = D3D12Renderer::Context->DeviceResources->Device->CreateRootSignature(
            0,
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize(),
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

#if 0
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
#endif		
	}


	HRESULT D3D12Shader::BuildPSO(CompilationSate* state, CD3DX12_PIPELINE_STATE_STREAM* pipelineStream)
	{
		pipelineStream->pRootSignature = state->rootSignature.Get();

		
		if ((m_ShaderTypes & ShaderType::Vertex) == ShaderType::Vertex) {
			pipelineStream->VS = CD3DX12_SHADER_BYTECODE((ID3DBlob*)state->vertexBlob.Get());
		}

		if ((m_ShaderTypes & ShaderType::Fragment) == ShaderType::Fragment) {
			pipelineStream->PS = CD3DX12_SHADER_BYTECODE((ID3DBlob*)state->fragmentBlob.Get());
		}

		if ((m_ShaderTypes & ShaderType::Geometry) == ShaderType::Geometry) {
			pipelineStream->GS = CD3DX12_SHADER_BYTECODE((ID3DBlob*)state->geometryBlob.Get());
		}

		if ((m_ShaderTypes & ShaderType::Compute) == ShaderType::Compute) {
			pipelineStream->CS = CD3DX12_SHADER_BYTECODE((ID3DBlob*)state->computeBlob.Get());
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
