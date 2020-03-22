#pragma once

#include "Hazel/Renderer/Shader.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Context.h"
#include "d3d12.h"

namespace Hazel {
	class D3D12Shader : public Shader
	{
	public:

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
		};

		D3D12Shader(const std::string& filepath, PipelineStateStream pipelineStream);
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
		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();
		// TODO: This is a very bad hack to get a "generic" recompile for OGL and DX12
		bool Recompile(void* pipelineStream = nullptr) override;
		
	private:
		std::string ReadFile(const std::string& filepath);

		struct CompilationSate {
			TComPtr<ID3DBlob> vertexBlob;
			TComPtr<ID3DBlob> fragmentBlob;
			TComPtr<ID3D12RootSignature> rootSignature;
			TComPtr<ID3D12PipelineState> pipelineState;
		};
		
		HRESULT Compile(const std::wstring& filepathW, LPCSTR entryPoint, LPCSTR profile, ID3DBlob** blob);
		HRESULT ExtractRootSignature(CompilationSate* state, TComPtr<ID3DBlob> shaderBlob);
		HRESULT BuildPSO(CompilationSate* state, PipelineStateStream* pipelineStream);
	private:
		TComPtr<ID3DBlob> m_VertexBlob;
		TComPtr<ID3DBlob> m_FragmentBlob;
		TComPtr<ID3D12RootSignature> m_RootSignature;
		TComPtr<ID3D12PipelineState> m_PipelineState;
		PipelineStateStream m_PipelineDesc;
		D3D12Context* m_Context;
		CompilationSate* m_CompilationState;

		std::string m_Name;
		std::string m_Path;
		
	};
}
