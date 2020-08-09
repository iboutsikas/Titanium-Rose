#include "hzpch.h"

#include "Hazel/Core/Application.h"

#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/DDSTextureLoader/DDSTextureLoader.h"


namespace Hazel {
#pragma region D3D12Texture
	D3D12Texture::D3D12Texture(uint32_t width, uint32_t height, uint32_t depth, 
		uint32_t mips, bool isCube, D3D12_RESOURCE_STATES initialState, std::string id) :
		m_Width(width),
		m_Height(height),
		m_Depth(depth),
		m_MipLevels(mips),
		m_IsCube(isCube),
		m_CurrentState(initialState),
		m_Identifier(id),
		m_Resource(nullptr)
	{

	}

	void D3D12Texture::Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		batch.GetCommandList()->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_Resource.Get(),
				from,
				to
			)
		);
		m_CurrentState = to;
	}

	void D3D12Texture::Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES to)
	{
		if (to == m_CurrentState)
			return;
		this->Transition(batch, m_CurrentState, to);
	}

	void D3D12Texture::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		commandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				m_Resource.Get(),
				from,
				to
			)
		);
		m_CurrentState = to;
	}

	void D3D12Texture::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES to)
	{
		if (to == m_CurrentState)
			return;
		this->Transition(commandList, m_CurrentState, to);
	}

	void D3D12Texture::SetCurrentState(D3D12_RESOURCE_STATES state)
	{ 
		m_CurrentState = state;
	}
#pragma endregion

#pragma region D3D12Texture2D
	D3D12Texture2D::D3D12Texture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips)
		: D3D12Texture(width, height, 1, mips, false, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, id),
		m_FeedbackMap(nullptr)
	{
	}
	
	void D3D12Texture2D::SetData(D3D12ResourceBatch& batch, void* data, uint32_t size)
	{
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = data;
		subresourceData.RowPitch = (uint64_t)m_Width * 4 * sizeof(uint8_t);
		subresourceData.SlicePitch = subresourceData.RowPitch * m_Height;

		batch.Upload(m_Resource.Get(), 0, &subresourceData, 1);
	}

	Ref<D3D12Texture2D> D3D12Texture2D::CreateVirtualTexture(D3D12ResourceBatch& batch, TextureCreationOptions& opts)
	{
		HZ_CORE_ASSERT(opts.Depth == 1, "2D textures should have a depth of 1");

		if (!opts.Path.empty() && opts.Name.empty())
		{
			HZ_CORE_ASSERT(false, "Virtual textures cannot have a path, but they need a name");
		}

		Ref<D3D12VirtualTexture2D> ret = CreateRef<D3D12VirtualTexture2D>(
			opts.Name,
			opts.Width,
			opts.Height,
			opts.MipLevels
		);
		auto device = batch.GetDevice();

		D3D12_RESOURCE_DESC desc = {};
		desc.MipLevels = opts.MipLevels;
		desc.Format = opts.Format;
		desc.Width = opts.Width;
		desc.Height = opts.Height;
		desc.Flags = opts.Flags;
		desc.DepthOrArraySize = opts.Depth;
		desc.SampleDesc = { 1, 0 };
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		
		D3D12::ThrowIfFailed(device->CreateReservedResource(
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
		));

		std::wstring wPath(opts.Name.begin(), opts.Name.end());

		ret->m_Resource->SetName(wPath.c_str());

		UINT subresourceCount = desc.MipLevels;
		ret->m_Tilings.resize(subresourceCount);

		batch.GetDevice()->GetResourceTiling(
			ret->m_Resource.Get(), &ret->m_NumTiles, &ret->m_MipInfo,
			&ret->m_TileShape, &subresourceCount, 0, &ret->m_Tilings[0]);

		ret->m_TileAllocations.resize(subresourceCount);

		for (uint32_t i = 0; i < subresourceCount; i++)
		{
			auto& allocationVector = ret->m_TileAllocations[i];
			auto& dims = ret->m_Tilings[i];

			allocationVector.resize((uint64_t)dims.WidthInTiles * dims.HeightInTiles);

			for (uint32_t y = 0; y < dims.HeightInTiles; ++y)
			{
				for (uint32_t x = 0; x < dims.WidthInTiles; ++x)
				{
					uint32_t index =(y * dims.WidthInTiles + x);
					allocationVector[index].Mapped = false;
					allocationVector[index].ResourceCoordinate = CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i);
				}
			}

		}

		return ret;
	}

	Ref<D3D12Texture2D> D3D12Texture2D::CreateCommittedTexture(D3D12ResourceBatch& batch, TextureCreationOptions& opts)
	{
		HZ_CORE_ASSERT(opts.Depth == 1, "2D textures should have a depth of 1");

		Ref<D3D12Texture2D> ret = nullptr;
		// We need to load from a file
		if (!opts.Path.empty())
		{
			std::string extension = opts.Path.substr(opts.Path.find_last_of(".") + 1);

			if (extension == "dds")
			{
				ret = LoadFromDDS(batch, opts);
			}
			else
			{
				Ref<Image> img = Image::FromFile(opts.Path);

				//opts.Format = img->IsHdr() ? DXGI_FORMAT_R16G16B16A16_FLOAT : opts.Format;
				//opts.MipLevels = 
				ret = LoadFromImage(img, batch, opts);
			}
			std::wstring wPath(opts.Path.begin(), opts.Path.end());
			ret->m_Resource->SetName(wPath.c_str());
		}
		// We create an in memory texture
		else if (!opts.Name.empty())
		{
			ret = CreateRef<D3D12CommittedTexture2D>(
				opts.Name,
				opts.Width,
				opts.Height,
				opts.MipLevels
			);

			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = opts.MipLevels;
			textureDesc.Format = opts.Format;
			textureDesc.Width = opts.Width;
			textureDesc.Height = opts.Height;
			textureDesc.Flags = opts.Flags;
			textureDesc.DepthOrArraySize = opts.Depth;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			Hazel::D3D12::ThrowIfFailed(batch.GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
				&textureDesc,
				ret->m_CurrentState,
				opts.IsDepthStencil ? &CD3DX12_CLEAR_VALUE(opts.Format, 1.0f, 0) : nullptr,
				IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
			));
			std::wstring wName(opts.Name.begin(), opts.Name.end());
			ret->m_Resource->SetName(wName.c_str());
		}
		else
		{
			HZ_CORE_ASSERT(false, "Commited textures need to have a path or a name");
		}
		return ret;
	}

	Ref<D3D12FeedbackMap> D3D12Texture2D::CreateFeedbackMap(D3D12ResourceBatch& batch, Ref<D3D12Texture2D> texture)
	{
		auto resource = texture->GetResource();
		auto desc = resource->GetDesc();
		uint32_t tiles_x = 1;
		uint32_t tiles_y = 1;

		if (texture->IsVirtual()) 
		{
			// We need to get tiling info from the device.
			UINT numTiles = 0;
			D3D12_TILE_SHAPE tileShape = {};
			UINT subresourceCount = texture->GetMipLevels();
			D3D12_PACKED_MIP_INFO mipInfo;
			std::vector<D3D12_SUBRESOURCE_TILING> tilings(subresourceCount);

			batch.GetDevice()->GetResourceTiling(
				resource, &numTiles, &mipInfo,
				&tileShape, &subresourceCount, 0, &tilings[0]);

			tiles_x = tilings[0].WidthInTiles;
			tiles_y = tilings[0].HeightInTiles;
		}
		else 
		{
			// D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT = 64K
			uint32_t factor = (desc.Height * desc.Width) / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			tiles_x = desc.Width / factor;
			tiles_y = desc.Height / factor;
		}

		// TODO: The feedback map is hardcoded to always be uint32_t now. Might have to change it later.
		// The constructor already supports it.
		auto ret = CreateRef<D3D12FeedbackMap>(
						batch.GetDevice(),
						tiles_x, tiles_y, sizeof(uint32_t)
					);
		auto name = texture->GetIdentifier() + "-feedback";
		std::wstring wname(name.begin(), name.end());

		ret->GetResource()->SetName(wname.c_str());

		return ret;
	}
	Ref<D3D12Texture2D> D3D12Texture2D::LoadFromDDS(D3D12ResourceBatch& batch, TextureCreationOptions& opts)
	{
		TComPtr<ID3D12Resource> resource = nullptr;
		std::unique_ptr<uint8_t[]> ddsData = nullptr;
		bool isCube;
		DirectX::DDS_ALPHA_MODE alphaMode;
		std::vector<D3D12_SUBRESOURCE_DATA> subData;

		std::wstring wPath(opts.Path.begin(), opts.Path.end());

		// Leaves resource in COPY_DEST state
		DirectX::LoadDDSTextureFromFile(
			batch.GetDevice().Get(),
			wPath.c_str(),
			resource.GetAddressOf(),
			ddsData,
			subData,
			0,
			&alphaMode,
			&isCube
		);

		auto desc = resource->GetDesc();
		// HZ_CORE_ASSERT(desc.Format == opts.Format, "The format requested does not match the format in the file");

		Ref<D3D12CommittedTexture2D> ret = CreateRef<D3D12CommittedTexture2D>(
			opts.Path,
			desc.Width,
			desc.Height,
			desc.MipLevels
			);
		ret->m_Resource.Swap(resource);
		ret->m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
		batch.Upload(ret->m_Resource.Get(), 0, subData.data(), subData.size());
		return ret;

	}

	Ref<D3D12Texture2D> D3D12Texture2D::LoadFromImage(Ref<Image>& image, D3D12ResourceBatch& batch, TextureCreationOptions& opts)
	{
		TComPtr<ID3D12Resource> resource = nullptr;
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = image->GetWidth();
		desc.Height = image->GetHeight();
		desc.DepthOrArraySize = 1;
		desc.MipLevels = opts.MipLevels;
		desc.Format = opts.Format;
		desc.SampleDesc.Count = 1;
		desc.Flags = opts.Flags;

		D3D12::ThrowIfFailed(batch.GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(resource.GetAddressOf())
		));

		Ref<D3D12CommittedTexture2D> ret = CreateRef<D3D12CommittedTexture2D>(
			opts.Path,
			desc.Width,
			desc.Height,
			desc.MipLevels
		);

		ret->m_Resource.Swap(resource);
		ret->m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

		D3D12_SUBRESOURCE_DATA data = { };
		data.pData = image->Bytes<void>();
		data.RowPitch = (uint64_t)image->GetWidth() * image->BytesPerPixel();
		data.SlicePitch = data.RowPitch * image->GetHeight();

		batch.TrackImage(image);
		batch.Upload(ret->m_Resource.Get(), 0, &data, 1);

		return ret;
	}
#pragma endregion

#pragma region D3D12VirtualTexture2D
	D3D12VirtualTexture2D::D3D12VirtualTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips)
		: D3D12Texture2D(id, width, height, mips),
		m_NumTiles(0),
		m_TileShape({}),
		m_MipInfo({}),
		m_CachedMipLevels({0, mips - 1})
	{
	}

	D3D12Texture2D::MipLevels D3D12VirtualTexture2D::ExtractMipsUsed()
	{
		HZ_CORE_ASSERT(m_FeedbackMap != nullptr, "Virtual textures should have a feedback map");

		auto dims = m_FeedbackMap->GetDimensions();

		uint32_t* data = m_FeedbackMap->GetData<uint32_t*>();

		uint32_t finest_mip = m_MipLevels;
		uint32_t coarsest_mip = 0;
		for (int y = 0; y < dims.y; y++)
		{
			for (int x = 0; x < dims.x; x++)
			{
				uint32_t mip = data[y * dims.x + x];

				if (mip < finest_mip) {
					finest_mip = mip;
				}

				if (mip > coarsest_mip) {
					coarsest_mip = mip;
				}
			}
		}
		m_CachedMipLevels.FinestMip = (finest_mip == m_MipLevels) ? finest_mip - 1 : finest_mip;
		m_CachedMipLevels.CoarsestMip = (coarsest_mip == m_MipLevels) ? coarsest_mip - 1 : coarsest_mip;


		return m_CachedMipLevels;
	}

	D3D12Texture2D::MipLevels D3D12VirtualTexture2D::GetMipsUsed()
	{
		return m_CachedMipLevels;
	}
#pragma endregion

#pragma region D3D12CommittedTexture2D
	D3D12CommittedTexture2D::D3D12CommittedTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips)
		: D3D12Texture2D(id, width, height, mips)
	{
	}
#pragma endregion

#pragma region D3D12TextureCube
	Ref<D3D12TextureCube> D3D12TextureCube::Create(D3D12ResourceBatch& batch, TextureCreationOptions& opts)
	{
		Ref<D3D12TextureCube> ret = nullptr;

		if (!opts.Path.empty())
		{
			TComPtr<ID3D12Resource> resource = nullptr;
			std::unique_ptr<uint8_t[]> ddsData = nullptr;
			bool isCube;
			DirectX::DDS_ALPHA_MODE alphaMode;
			std::vector<D3D12_SUBRESOURCE_DATA> subData;

			std::wstring wPath(opts.Path.begin(), opts.Path.end());

			// Leaves resource in COPY_DEST state
			DirectX::LoadDDSTextureFromFile(
				batch.GetDevice().Get(),
				wPath.c_str(),
				resource.GetAddressOf(),
				ddsData,
				subData,
				0,
				&alphaMode,
				&isCube
			);

			HZ_CORE_ASSERT(isCube, "File is not a cube texture");

			auto desc = resource->GetDesc();

			ret = CreateRef<D3D12TextureCube>(
				desc.Width,
				desc.Height,
				desc.DepthOrArraySize,
				desc.MipLevels,
				opts.Path
			);
			ret->m_Resource.Swap(resource);
			ret->m_Resource->SetName(wPath.c_str());
			ret->m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
			batch.Upload(ret->m_Resource.Get(), 0, subData.data(), subData.size());
		}
		else if (!opts.Name.empty())
		{
			HZ_CORE_ASSERT(opts.Depth == 6, "Cube textures should have a depth of 6 (6 faces)");

			ret = CreateRef<D3D12TextureCube>(
				opts.Width, 
				opts.Height, 
				opts.Depth,
				opts.MipLevels,
				opts.Name 
			);

			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = opts.MipLevels;
			textureDesc.Format = opts.Format;
			textureDesc.Width = opts.Width;
			textureDesc.Height = opts.Height;
			textureDesc.Flags = opts.Flags;
			textureDesc.DepthOrArraySize = opts.Depth;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			Hazel::D3D12::ThrowIfFailed(batch.GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
				&textureDesc,
				ret->m_CurrentState,
				nullptr,
				IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
			));

			std::wstring wName(opts.Name.begin(), opts.Name.end());
			ret->m_Resource->SetName(wName.c_str());
		}
		else
		{
			HZ_CORE_ASSERT(false, "Commited textures need to have a path or a name");
		}

		return ret;
	}
	D3D12TextureCube::D3D12TextureCube(uint32_t width, uint32_t height, uint32_t depth, 
		uint32_t mips, std::string id) :
		D3D12Texture(width, height, depth, mips, true, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, id)
	{
	}
#pragma endregion
	
}

