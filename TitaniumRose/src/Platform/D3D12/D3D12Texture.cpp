#include "trpch.h"

#include "TitaniumRose/Core/Application.h"
#include "Platform/D3D12/CommandContext.h"
#include "Platform/D3D12/D3D12Context.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/DDSTextureLoader/DDSTextureLoader.h"


const float clrclr[4] = { 0.0f, 0.0f, 0.0f, 0.0f };


namespace Roses {
	Texture::Texture(uint32_t width, uint32_t height, uint32_t depth, 
		uint32_t mips, bool isCube, D3D12_RESOURCE_STATES initialState, std::string id) :
		GpuBuffer(width, height, depth, id, initialState),
		m_MipLevels(mips),
		m_IsCube(isCube),
		m_ResourceAllocationInfo({0})
	{

	}

	void Texture::UpdateFromDescription()
	{
		HZ_CORE_ASSERT(m_Resource != nullptr, "Resource is not initialized");
        auto desc = m_Resource->GetDesc();

        m_Width = static_cast<uint32_t>(desc.Width);
        m_Height = desc.Height;
        m_Depth = desc.DepthOrArraySize;
        m_MipLevels = desc.MipLevels;
		
		m_ResourceAllocationInfo = D3D12Renderer::GetDevice()->GetResourceAllocationInfo(0, 1, &desc);
	}

	Texture2D::Texture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips)
		: Texture(width, height, 1, mips, false, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, id),
		m_FeedbackMap(nullptr)
	{
	}

	Ref<Texture2D> Texture2D::CreateVirtualTexture(TextureCreationOptions& opts)
	{
		HZ_CORE_ASSERT(opts.Depth == 1, "2D textures should have a depth of 1");

		if (!opts.Path.empty() && opts.Name.empty())
		{
			HZ_CORE_ASSERT(false, "Virtual textures cannot have a path, but they need a name");
		}

		Ref<VirtualTexture2D> ret = CreateRef<VirtualTexture2D>(
			opts.Name,
			opts.Width,
			opts.Height,
			opts.MipLevels
		);

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
		
		D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateReservedResource(
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
		));

		ret->SetName(opts.Name);
		ret->UpdateFromDescription();
		UINT subresourceCount = desc.MipLevels;
		ret->m_Tilings.resize(subresourceCount);

		D3D12Renderer::GetDevice()->GetResourceTiling(
			ret->m_Resource.Get(), &ret->m_NumTiles, &ret->m_MipInfo,
			&ret->m_TileShape, &subresourceCount, 0, &ret->m_Tilings[0]);
		return ret;
	}

	Ref<Texture2D> Texture2D::CreateCommittedTexture(TextureCreationOptions& opts)
	{
		HZ_CORE_ASSERT(opts.Depth == 1, "2D textures should have a depth of 1");

		Ref<Texture2D> ret = nullptr;
		// We need to load from a file
		if (!opts.Path.empty())
		{
			std::string extension = opts.Path.substr(opts.Path.find_last_of(".") + 1);

			if (extension == "dds")
			{
				ret = LoadFromDDS(opts);
			}
			else
			{
				Ref<Image> img = Image::FromFile(opts.Path);

				//opts.Format = img->IsHdr() ? DXGI_FORMAT_R16G16B16A16_FLOAT : opts.Format;
				//opts.MipLevels = 
				ret = LoadFromImage(img, opts);
			}
			ret->SetName(opts.Path);
		}
		// We create an in memory texture
		else if (!opts.Name.empty())
		{
			ret = CreateRef<CommittedTexture2D>(
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


			D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

			if (opts.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
				heapFlags = D3D12_HEAP_FLAG_NONE;
			}

			if (opts.IsDepthStencil) {
				ret->BypassAndSetState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
			}

			Roses::D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				heapFlags,
				&textureDesc,
				ret->m_CurrentState,
				opts.IsDepthStencil ? &CD3DX12_CLEAR_VALUE(opts.Format, 1.0f, 0) : nullptr,
				IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
			));

			ret->SetName(opts.Name);
			ret->UpdateFromDescription();
		}
		else
		{
			HZ_CORE_ASSERT(false, "Commited textures need to have a path or a name");
		}
		return ret;
	}

	D3D12FeedbackMap* Texture2D::CreateFeedbackMap(Texture2D& texture)
	{
		auto resource = texture.GetResource();
		auto desc = resource->GetDesc();
		uint32_t tiles_x = 1;
		uint32_t tiles_y = 1;

		if (texture.IsVirtual()) 
		{
			// We need to get tiling info from the device.
			UINT numTiles = 0;
			D3D12_TILE_SHAPE tileShape = {};
			UINT subresourceCount = texture.GetMipLevels();
			D3D12_PACKED_MIP_INFO mipInfo;
			std::vector<D3D12_SUBRESOURCE_TILING> tilings(subresourceCount);

			D3D12Renderer::GetDevice()->GetResourceTiling(
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
		auto ret = new D3D12FeedbackMap(D3D12Renderer::GetDevice(), tiles_x, tiles_y, sizeof(uint32_t));
		auto name = texture.GetIdentifier() + "-feedback";

		ret->SetName(name);

		return ret;
	}
	
	Ref<Texture2D> Texture2D::LoadFromDDS(TextureCreationOptions& opts)
	{
		bool isCube;
		DirectX::DDS_ALPHA_MODE alphaMode;
		std::vector<D3D12_SUBRESOURCE_DATA> subData;

		std::ifstream file(opts.Path, std::ios::binary);

		std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		
		CommittedTexture2D* ret = new CommittedTexture2D(
            opts.Path,
			opts.Width,
			opts.Height,
			opts.MipLevels
		); 

        DirectX::LoadDDSTextureFromMemory(
            D3D12Renderer::GetDevice(),
            bytes.data(), bytes.size(), ret->m_Resource.GetAddressOf(),
            subData, 0, &alphaMode, &isCube
        );

		// LoadDDSTextureFromMemory leaves the resource as D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		ret->BypassAndSetState(D3D12_RESOURCE_STATE_COPY_DEST);
		ret->SetName(opts.Path);
		// The resource is left in the copy_dest state from LoadDDSTextureFromMemory
		CommandContext::InitializeTexture(*ret, subData.size(), subData.data());
		ret->UpdateFromDescription();

		return Ref<Texture2D>(ret);
}

	Ref<Texture2D> Texture2D::LoadFromImage(Ref<Image>& image, TextureCreationOptions& opts)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = image->GetWidth();
		desc.Height = image->GetHeight();
		desc.DepthOrArraySize = 1;
		desc.MipLevels = opts.MipLevels;
		desc.Format = opts.Format;
		desc.SampleDesc.Count = 1;
		desc.Flags = opts.Flags;

        CommittedTexture2D* ret = new CommittedTexture2D(
            opts.Path,
            desc.Width,
            desc.Height,
            desc.MipLevels
        );

		D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
		));

		ret->BypassAndSetState(D3D12_RESOURCE_STATE_COPY_DEST);

		D3D12_SUBRESOURCE_DATA data = { };
		data.pData = image->Bytes<void>();
		data.RowPitch = (uint64_t)image->GetWidth() * image->BytesPerPixel();
		data.SlicePitch = data.RowPitch * image->GetHeight();
		CommandContext::InitializeTexture(*ret, 1, &data);

		ret->UpdateFromDescription();
				
		return Ref<CommittedTexture2D>(ret);
	}

	VirtualTexture2D::VirtualTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips)
		: Texture2D(id, width, height, mips),
		m_NumTiles(0),
		m_TileShape({}),
		m_MipInfo({}),
		m_CachedMipLevels({0, mips - 1})
	{
	}

	VirtualTexture2D::~VirtualTexture2D()
	{
	/*	auto thing = this->shared_from_this();
		D3D12Renderer::TilePool->ReleaseTexture(thing, D3D12Renderer::Context->DeviceResources->CommandQueue);*/
	}

	Texture2D::MipLevelsUsed VirtualTexture2D::ExtractMipsUsed()
	{
		HZ_CORE_ASSERT(m_FeedbackMap != nullptr, "Virtual textures should have a feedback map");

		auto dims = m_FeedbackMap->GetDimensions();

		uint32_t* data = m_FeedbackMap->GetData();

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

				if (mip > coarsest_mip && mip != m_MipLevels) {
					coarsest_mip = mip;
				}
			}
		}
		m_CachedMipLevels.FinestMip = (finest_mip == m_MipLevels) ? finest_mip - 1 : finest_mip;
		m_CachedMipLevels.CoarsestMip = (coarsest_mip == m_MipLevels) ? coarsest_mip - 1 : coarsest_mip;

		return m_CachedMipLevels;
	}

	Texture2D::MipLevelsUsed VirtualTexture2D::GetMipsUsed()
	{
		return m_CachedMipLevels;
	}

	glm::ivec3 VirtualTexture2D::GetTileDimensions(uint32_t subresource) const
	{
		HZ_CORE_ASSERT(subresource < m_Tilings.size(), "Subresource is out of bounds");
		auto& t = m_Tilings[subresource];
		return glm::ivec3(t.WidthInTiles, t.HeightInTiles, t.DepthInTiles);
	}

	uint64_t VirtualTexture2D::GetTileUsage()
	{
		return D3D12Renderer::TilePool->GetTilesUsed(*this);
	}

	CommittedTexture2D::CommittedTexture2D(std::string id, uint32_t width, uint32_t height, uint32_t mips)
		: Texture2D(id, width, height, mips)
	{
	}


#pragma region D3D12TextureCube
	Ref<D3D12TextureCube> D3D12TextureCube::Initialize(TextureCreationOptions& opts)
	{
		Ref<D3D12TextureCube> ret = nullptr;

		if (!opts.Path.empty())
		{
            std::ifstream file(opts.Path, std::ios::binary);

            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			D3D12TextureCube* ret = new D3D12TextureCube(
                opts.Width,
				opts.Height,
				opts.Depth,
				opts.MipLevels,
                opts.Path
            );

            bool isCube;
            DirectX::DDS_ALPHA_MODE alphaMode;
            std::vector<D3D12_SUBRESOURCE_DATA> subData;


            DirectX::LoadDDSTextureFromMemory(
                D3D12Renderer::GetDevice(),
                bytes.data(), bytes.size(), ret->m_Resource.GetAddressOf(),
                subData, 0, &alphaMode, &isCube
            );

			HZ_CORE_ASSERT(isCube, "File is not a cube texture");

			ret->UpdateFromDescription();

            ret->SetName(opts.Path);
			ret->BypassAndSetState(D3D12_RESOURCE_STATE_COPY_DEST);
			CommandContext::InitializeTexture(*ret, subData.size(), subData.data());
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

			Roses::D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
				&textureDesc,
				ret->m_CurrentState,
				nullptr,
				IID_PPV_ARGS(ret->m_Resource.GetAddressOf())
			));

			ret->SetName(opts.Name);
		}
		else
		{
			HZ_CORE_ASSERT(false, "Commited textures need to have a path or a name");
		}

		return ret;
	}
	D3D12TextureCube::D3D12TextureCube(uint32_t width, uint32_t height, uint32_t depth, 
		uint32_t mips, std::string id) :
		Texture(width, height, depth, mips, true, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, id)
	{
	}
#pragma endregion

}

