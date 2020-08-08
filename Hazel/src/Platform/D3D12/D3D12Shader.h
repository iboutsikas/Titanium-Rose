#pragma once

#include "Hazel/Renderer/Shader.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "d3d12.h"

#define DECLARE_SHADER(name)\
    static constexpr char* ShaderName = name;\
    static constexpr char* ShaderPath = "assets/shaders/" name ".hlsl";

struct CD3DX12_PIPELINE_STATE_STREAM;

namespace Hazel {
	
	
	class D3D12Shader : public Shader
	{
	public:
		

		//struct PipelineStateStream
		//{
		//	CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		//	CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		//	CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		//	CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		//	CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		//	CD3DX12_PIPELINE_STATE_STREAM_GS GS;
		//	CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		//	CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		//	CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		//	CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
		//};

		D3D12Shader(const std::string& filepath, CD3DX12_PIPELINE_STATE_STREAM pipelineStream, ShaderType shaderTypes = VertexAndFragment);
		D3D12Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~D3D12Shader();

		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		virtual void SetInt(const std::string& name, int value) override {};
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override {};
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override {};
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override {};

		virtual const std::string& GetName() const override { return m_Name; }
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
			TComPtr<ID3DBlob> vertexBlob;
			TComPtr<ID3DBlob> fragmentBlob;
			TComPtr<ID3DBlob> geometryBlob;
			TComPtr<ID3DBlob> computeBlob;
			TComPtr<ID3D12RootSignature> rootSignature;
			TComPtr<ID3D12PipelineState> pipelineState;
		};

		bool RecompileCompute(CD3DX12_PIPELINE_STATE_STREAM* pipelineStream);
		bool RecompileGraphics(CD3DX12_PIPELINE_STATE_STREAM* pipelineStream);


		HRESULT Compile(const std::wstring& filepathW, LPCSTR entryPoint, LPCSTR profile, ID3DBlob** blob);
		HRESULT ExtractRootSignature(CompilationSate* state, TComPtr<ID3DBlob> shaderBlob);
		HRESULT BuildPSO(CompilationSate* state, CD3DX12_PIPELINE_STATE_STREAM* pipelineStream);
	private:
		TComPtr<ID3DBlob> m_VertexBlob;
		TComPtr<ID3DBlob> m_FragmentBlob;
		TComPtr<ID3DBlob> m_GeometryBlob;
		TComPtr<ID3D12RootSignature> m_RootSignature;
		TComPtr<ID3D12PipelineState> m_PipelineState;
		CD3DX12_PIPELINE_STATE_STREAM m_PipelineDesc;
		Ref<CompilationSate> m_CompilationState;

		std::string m_Name;
		std::string m_Path;
		ShaderType m_ShaderTypes;
	};
}
