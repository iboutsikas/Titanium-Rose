#include "hzpch.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/DDSTextureLoader/DDSTextureLoader.h"

//#include "stb_image.h"

namespace Hazel {
	D3D12Texture2D::D3D12Texture2D(uint32_t width, uint32_t height, uint32_t mipLevels)
		: m_Width(width), m_Height(height), m_MipLevels(mipLevels)
	{
		HZ_PROFILE_FUNCTION();

		m_Context = static_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = m_MipLevels;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		Hazel::D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&textureDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&m_CommittedResource)));

		m_CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}


	D3D12Texture2D::D3D12Texture2D(const std::string& path)
		: m_Path(path), m_MipLevels(1)
	{
		HZ_PROFILE_FUNCTION();
		m_Context = static_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());

		std::wstring filepath(path.begin(), path.end());

		std::unique_ptr<uint8_t[]> ddsData = nullptr;
		bool isCube;
		DirectX::DDS_ALPHA_MODE alphaMode;

		DirectX::LoadDDSTextureFromFile(
			m_Context->DeviceResources->Device.Get(),
			filepath.c_str(),
			&m_CommittedResource,
			ddsData,
			m_SubData,
			0,
			&alphaMode,
			&isCube
		);

		m_MipLevels = m_SubData.size();
		m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

		const uint64_t uploadSize = GetRequiredIntermediateSize(m_CommittedResource.Get(), 0, m_MipLevels);

		D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_UploadResource.GetAddressOf())));

		UpdateSubresources(m_Context->DeviceResources->CommandList.Get(),
			m_CommittedResource.Get(), m_UploadResource.Get(),
			0, 0, m_SubData.size(), m_SubData.data());
	}

	void D3D12Texture2D::SetData(void* data, uint32_t size)
	{
		if (!m_UploadResource) {

			const uint64_t uploadSize = GetRequiredIntermediateSize(m_CommittedResource.Get(), 0, 1);
			D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_UploadResource.GetAddressOf())));
		}

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = data;
		subresourceData.RowPitch = m_Width * 4 * sizeof(uint8_t);
		subresourceData.SlicePitch = subresourceData.RowPitch * m_Height;

		UpdateSubresources(m_Context->DeviceResources->CommandList.Get(),
			m_CommittedResource.Get(), m_UploadResource.Get(),
			0, 0, 1, &subresourceData);
	}

	void D3D12Texture2D::Bind(uint32_t slot) const
	{
	}

	void D3D12Texture2D::Transition(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		auto cmdList = m_Context->DeviceResources->CommandList;

		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_CommittedResource.Get(), 
				from, 
				to
			)
		);
		m_CurrentState = to;
	}
	void D3D12Texture2D::Transition(D3D12_RESOURCE_STATES to)
	{
		if (to == m_CurrentState)
			return;

		auto cmdList = m_Context->DeviceResources->CommandList;

		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_CommittedResource.Get(),
				m_CurrentState,
				to
			)
		);

		m_CurrentState = to;
	}

	void D3D12Texture2D::TransitionFeedback(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		auto cmdList = m_Context->DeviceResources->CommandList;

		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_CommittedResource.Get(),
				from,
				to
			)
		);
		m_FeedbackState = to;
	}

	void D3D12Texture2D::TransitionFeedback(D3D12_RESOURCE_STATES to)
	{
		if (to == m_FeedbackState)
			return;

		auto cmdList = m_Context->DeviceResources->CommandList;

		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_CommittedResource.Get(),
				m_FeedbackState,
				to
			)
		);

		m_FeedbackState = to;
	}

	void D3D12Texture2D::CreateFeedbackResource(UINT TileWidth, UINT TileHeight)
	{
		if (!m_CommittedResource) {
			return;
		}

		m_FeedbackDims.x = std::ceil((float)m_Width / TileWidth);
		m_FeedbackDims.y = std::ceil((float)m_Height / TileHeight);
		m_FeedbackSize = D3D12::CalculateConstantBufferSize(m_FeedbackDims.x * m_FeedbackDims.y);

		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_FeedbackSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&m_FeedbackResource)
		));
	}


	void D3D12Texture2D::DebugNameResource(std::wstring& name)
	{
		m_CommittedResource->SetName(name.c_str());
	}
}

