#include "DeferedTexturePass.h"
#include "Vertex.h"

#include "Platform/D3D12/D3D12Helpers.h"

DeferedTexturePass::DeferedTexturePass(Hazel::D3D12Context* ctx)
	: Hazel::D3D12RenderPass(3)
{
	// Shader
	{
		m_PublicShader = Hazel::CreateRef<Hazel::D3D12Shader>("assets/shaders/TextureShader.hlsl");
	}

	// Root Signature
	{

		// We need an abstract version of this too. Should probably be tied to some kind of render pass or something
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(ctx->DeviceResources->Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;;

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		// A single 32-bit constant root parameter that is used by the vertex shader.
		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsConstantBufferView(0);
		rootParameters[1].InitAsDescriptorTable(1, ranges, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

		// Serialize the root signature.
		Hazel::TComPtr<ID3DBlob> rootSignatureBlob;
		Hazel::TComPtr<ID3DBlob> errorBlob;
		Hazel::D3D12::ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
			featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
		// Create the root signature.
		Hazel::D3D12::ThrowIfFailed(ctx->DeviceResources->Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_PublicRootSignature)));
	}

	// Pipeline State 
	{
		// We need to get this from the VertexBuffer in generic way
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, Color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, UV), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;


		CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
		//rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		rasterizer.FrontCounterClockwise = TRUE;
		rasterizer.CullMode = D3D12_CULL_MODE_BACK;
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
		} pipelineStateStream;

		pipelineStateStream.pRootSignature = m_PublicRootSignature.Get();
		pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(m_PublicShader->GetVertexBlob().Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(m_PublicShader->GetFragmentBlob().Get());
		pipelineStateStream.DSVFormat = DXGI_FORMAT_UNKNOWN;
		pipelineStateStream.RTVFormats = rtvFormats;
		pipelineStateStream.Rasterizer = CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(rasterizer);

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		Hazel::D3D12::ThrowIfFailed(
			ctx->DeviceResources->Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PublicPipelineState))
		);
	}
}

void DeferedTexturePass::Update(Hazel::Timestep ts)
{
	Hazel::D3D12RenderPass::Update(ts);
}

void DeferedTexturePass::Process(Hazel::D3D12Context* ctx)
{
	if (Target == nullptr) return;
	
	D3D12_VIEWPORT vp = { 0.0f, 0.0f, Target->GetWidth(), Target->GetHeight(), 0.0f, 1.0f };
	D3D12_RECT rect = { 0.0f, 0.0f, Target->GetWidth(), Target->GetHeight() };

	auto cmdList = ctx->DeviceResources->CommandList;

	cmdList->SetGraphicsRootSignature(m_PublicRootSignature.Get());
	cmdList->SetPipelineState(m_PublicPipelineState.Get());

	cmdList->RSSetViewports(1, &vp);
	cmdList->RSSetScissorRects(1, &rect);
}

void DeferedTexturePass::Recompile(Hazel::D3D12Context* ctx)
{
}
