#pragma once

#include "TitaniumRose/Renderer/Shader.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"

#include <d3d12.h>
#include <dxcapi.h>


#define DECLARE_SHADER_NAMED(name, variablename)\
    static constexpr char* ShaderName##variablename = name;\
    static constexpr char* ShaderPath##variablename = "assets/shaders/" name ".hlsl";

#define DECLARE_SHADER(name) DECLARE_SHADER_NAMED(#name, ##name) 

struct CD3DX12_PIPELINE_STATE_STREAM;

namespace Roses {
	
	
	class D3D12Shader : public Shader
	{
	public:

		D3D12Shader(const std::string& filepath, CD3DX12_PIPELINE_STATE_STREAM pipelineStream, ShaderType shaderTypes = VertexAndFragment);
		D3D12Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~D3D12Shader();

		virtual void UpdateReferences() override;

		inline TComPtr<ID3DBlob> GetVertexBlob() const { return m_VertexBlob; }
		inline TComPtr<ID3DBlob> GetFragmentBlob() const { return m_FragmentBlob; }
		inline TComPtr<ID3DBlob> GetGeometryBlob() const { return m_GeometryBlob; }
		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();

		inline bool ContainsShader(ShaderType shaderType) const { return static_cast<bool>(m_ShaderTypes & shaderType); }


		// TODO: This is a very bad hack to get a "generic" recompile for OGL and DX12
		bool Recompile(void* pipelineStream = nullptr) override;
		
	private:
		std::string ReadFile(const std::string& filepath);

		struct CompilationSate {
			TComPtr<IDxcBlob> vertexBlob;
			TComPtr<IDxcBlob> fragmentBlob;
			TComPtr<IDxcBlob> geometryBlob;
			TComPtr<IDxcBlob> computeBlob;
			TComPtr<ID3D12RootSignature> rootSignature;
			TComPtr<ID3D12PipelineState> pipelineState;
		};

		bool RecompileCompute(CD3DX12_PIPELINE_STATE_STREAM* pipelineStream);
		bool RecompileGraphics(CD3DX12_PIPELINE_STATE_STREAM* pipelineStream);


		HRESULT Compile(const std::wstring& filepathW, LPCWSTR entryPoint, LPCWSTR profile, IDxcBlob** blob);
		HRESULT ExtractRootSignature(CompilationSate* state, TComPtr<IDxcBlob> shaderBlob);
		HRESULT BuildPSO(CompilationSate* state, CD3DX12_PIPELINE_STATE_STREAM* pipelineStream);
	private:
		TComPtr<ID3DBlob> m_VertexBlob;
		TComPtr<ID3DBlob> m_FragmentBlob;
		TComPtr<ID3DBlob> m_GeometryBlob;
		TComPtr<ID3D12RootSignature> m_RootSignature;
		TComPtr<ID3D12PipelineState> m_PipelineState;
		CD3DX12_PIPELINE_STATE_STREAM m_PipelineDesc;
		Ref<CompilationSate> m_CompilationState;

		std::string m_Path;
		ShaderType m_ShaderTypes;
	};
}