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
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
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


	D3D12Texture2D::D3D12Texture2D(const std::string& path, ID3D12GraphicsCommandList* cmdList)
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

		auto desc = m_CommittedResource->GetDesc();
		m_MipLevels = desc.MipLevels;
		m_Width = desc.Width;
		m_Height = desc.Height;

		m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

		const uint64_t uploadSize = GetRequiredIntermediateSize(m_CommittedResource.Get(), 0, m_MipLevels);

		D3D12::ThrowIfFailed(m_Context->DeviceResources->Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_UploadResource.GetAddressOf())));

		UpdateSubresources(cmdList, m_CommittedResource.Get(), 
			m_UploadResource.Get(), 0, 0, m_SubData.size(),
			m_SubData.data());
	}

	D3D12Texture2D::D3D12Texture2D(const std::string& path)
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

		auto desc = m_CommittedResource->GetDesc();
		m_MipLevels = desc.MipLevels;
		m_Width = desc.Width;
		m_Height = desc.Height;

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
			0, 0, m_SubData.size(),	m_SubData.data());
	}

	void D3D12Texture2D::SetData(ID3D12GraphicsCommandList* cmdList, void* data, uint32_t size)
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

		UpdateSubresources(cmdList,
			m_CommittedResource.Get(), m_UploadResource.Get(),
			0, 0, 1, &subresourceData);
	}

	void D3D12Texture2D::SetData(void* data, uint32_t size)
	{
		auto cmdList = m_Context->DeviceResources->CommandList;

		this->SetData(cmdList.Get(), data, size);
	}

	void D3D12Texture2D::Bind(uint32_t slot) const
	{
	}

	void D3D12Texture2D::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_CommittedResource.Get(),
				from,
				to
			)
		);
		m_CurrentState = to;
	}

	void D3D12Texture2D::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES to)
	{
		if (to == m_CurrentState)
			return;
		this->Transition(cmdList, m_CurrentState, to);
	}

	void D3D12Texture2D::Transition(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		auto cmdList = m_Context->DeviceResources->CommandList;

		Transition(cmdList.Get(), from, to);
	}
	void D3D12Texture2D::Transition(D3D12_RESOURCE_STATES to)
	{
		if (to == m_CurrentState)
			return;
		this->Transition(m_CurrentState, to);
	}

	
	void D3D12Texture2D::CreateFeedbackResource(uint32_t TileWidth, uint32_t TileHeight)
	{
		if (!m_CommittedResource) {
			return;
		}

		uint32_t TilesX = std::ceil((float)m_Width / TileWidth);
		uint32_t TilesY = std::ceil((float)m_Height / TileHeight);
		
		m_FeedbackMap = CreateRef<D3D12FeedbackMap>(
			m_Context->DeviceResources->Device,
			TilesX, TilesY, sizeof(uint32_t)
		);
	}


	void D3D12Texture2D::DebugNameResource(std::wstring& name)
	{
		m_CommittedResource->SetName(name.c_str());
	}
	Ref<D3D12Texture2D> D3D12Texture2D::CreateVirtualTexture(uint32_t width, uint32_t height, uint32_t mipLevels)
	{
		auto ctx = static_cast<D3D12Context*>(Application::Get().GetWindow().GetContext());
		auto dev = ctx->DeviceResources->Device;

		auto tex = Hazel::CreateRef<D3D12Texture2D>();

		D3D12_RESOURCE_DESC reservedTextureDesc = {};
		reservedTextureDesc.MipLevels = mipLevels;
		reservedTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		reservedTextureDesc.Width = width;
		reservedTextureDesc.Height = height;
		reservedTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		reservedTextureDesc.DepthOrArraySize = 1;
		reservedTextureDesc.SampleDesc.Count = 1;
		reservedTextureDesc.SampleDesc.Quality = 0;
		reservedTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		reservedTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

		dev->CreateReservedResource(&reservedTextureDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&tex->m_CommittedResource));

		tex->m_Context = ctx;
		tex->m_Width = width;
		tex->m_Height = height;
		tex->m_MipLevels = mipLevels;
		tex->m_CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		tex->m_IsVirtual = true;

		return tex;
	}
}

