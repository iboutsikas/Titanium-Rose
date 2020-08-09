#include "MipMapPass.h"
#include "Platform/D3D12/D3D12Renderer.h"

static constexpr char* MipGenShaderPath = "assets/shaders/MipGenShader.hlsl";
static constexpr uint32_t MipsPerIteration = 4;


MipMapPass::MipMapPass(Hazel::D3D12Context* ctx)
	: D3D12RenderPass(ctx)
{
	Hazel::D3D12Shader::PipelineStateStream stream;
	if (Hazel::D3D12Renderer::ShaderLibrary->Exists(MipGenShaderPath))
	{
		m_Shader = Hazel::D3D12Renderer::ShaderLibrary->GetAs<Hazel::D3D12Shader>(MipGenShaderPath);
	}
	else
	{
		m_Shader = Hazel::CreateRef<Hazel::D3D12Shader>(MipGenShaderPath, stream, Hazel::ShaderType::Compute);
		Hazel::D3D12Renderer::ShaderLibrary->Add(m_Shader);
	}

	m_SRVHeap = ctx->DeviceResources->CreateDescriptorHeap(
		ctx->DeviceResources->Device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		MipMapPass::PassInputCount + 12,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);

	m_SRVHeap->SetName(L"MipMapPass::ResourceHeap");
}

void MipMapPass::Process(Hazel::D3D12Context* ctx, Hazel::GameObject* sceneRoot, Hazel::PerspectiveCamera& camera)
{
	//auto cmdList = ctx->DeviceResources->CommandList;
	auto device = ctx->DeviceResources->Device;
	Hazel::D3D12ResourceBatch batch(device.Get());

	auto cmdList = batch.Begin();
	PIXBeginEvent(cmdList.Get(), PIX_COLOR(1, 0, 1), "Mips Pass");

	{
		auto resource = m_Inputs[0];
		auto srcDesc = resource->GetResource()->GetDesc();
		uint32_t remainingMips = (srcDesc.MipLevels > PassData.SourceLevel) ?
			srcDesc.MipLevels - PassData.SourceLevel - 1 :
			srcDesc.MipLevels - 1;
		resource->Transition(cmdList.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		cmdList->SetComputeRootSignature(m_Shader->GetRootSignature());
		cmdList->SetPipelineState(m_Shader->GetPipelineState());

		cmdList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_SRVHeap->GetCPUDescriptorHandleForHeapStart(),
			1,
			ctx->GetSRVDescriptorSize()
		);

		//D3D12_SHADER_RESOURCE_VIEW_DESC srcViewDesc;
		//srcViewDesc.Format = srcDesc.Format;
		//srcViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//srcViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//srcViewDesc.Texture2D.MostDetailedMip = PassData.SourceLevel;
		//srcViewDesc.Texture2D.MipLevels = -1;
		//srcViewDesc.Texture2D.PlaneSlice = 0;


		//device->CreateShaderResourceView(
		//	resource->GetResource(),
		//	&srcViewDesc,
		//	handle
		//);

		//handle.Offset(1, m_Context->GetSRVDescriptorSize());

		// Looped
		uint32_t uavOffset = 1;
		for (uint32_t srcMip = PassData.SourceLevel; srcMip < srcDesc.MipLevels - 1; )
		{
			uint64_t srcWidth	= srcDesc.Width >> srcMip;
			uint32_t srcHeight	= srcDesc.Height >> srcMip;
			uint32_t dstWidth	= static_cast<uint32_t>(srcWidth >> 1);
			uint32_t dstHeight	= static_cast<uint32_t>(srcHeight >> 1);

			uint32_t mipCount = min(MipsPerIteration, remainingMips);
			mipCount = (srcMip + mipCount) >= srcDesc.MipLevels ? srcDesc.MipLevels - srcMip - 1 : mipCount;
			dstWidth = std::max<DWORD>(1, dstWidth);
			dstHeight = std::max<DWORD>(1, dstHeight);

			PassData.SourceLevel = srcMip;
			PassData.Levels = mipCount;
			PassData.TexelSize = glm::vec4({srcWidth, srcHeight, 1.0f / (float)dstWidth, 1.0f / (float)dstHeight});

			cmdList->SetComputeRoot32BitConstants(0, sizeof(PassData) / sizeof(uint32_t), &PassData, 0);
			// First descriptor is the src texture. We are not touching that one
			

			for (uint32_t mip = 0; mip < mipCount; mip++)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = srcDesc.Format;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

				ctx->DeviceResources->Device->CreateUnorderedAccessView(
					resource->GetResource(),
					nullptr,
					&uavDesc,
					handle
				);
				handle.Offset(1, ctx->GetSRVDescriptorSize());
			}

			if (mipCount < MipsPerIteration)
			{
				for (uint32_t mip = mipCount; mip < MipsPerIteration; mip++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
					uavDesc.Format = srcDesc.Format;
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uavDesc.Texture2D.MipSlice = 0;
					ctx->DeviceResources->Device->CreateUnorderedAccessView(
						nullptr,
						nullptr,
						&uavDesc,
						handle
					);
				handle.Offset(1, ctx->GetSRVDescriptorSize());
				}
			}

			cmdList->SetComputeRootDescriptorTable(1, m_SRVHeap->GetGPUDescriptorHandleForHeapStart());
			auto uavHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
				m_SRVHeap->GetGPUDescriptorHandleForHeapStart(),
				uavOffset,
				m_Context->GetSRVDescriptorSize()
			);

			cmdList->SetComputeRootDescriptorTable(2, uavHandle);

			auto x_count = Hazel::D3D12::RoundToMultiple(dstWidth, 8);
			auto y_count = Hazel::D3D12::RoundToMultiple(dstHeight, 8);

			cmdList->Dispatch(x_count, y_count, 1);
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(resource->GetResource()));
			srcMip += mipCount;
			uavOffset += mipCount;
		}
		
	}

	PIXEndEvent();
	batch.End(ctx->DeviceResources->CommandQueue.Get());
}
