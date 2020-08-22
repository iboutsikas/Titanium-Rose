#pragma once

#include <set>

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"


namespace Hazel 
{
	union TileAddress
	{
		uint32_t Address;
		struct {
			uint16_t Page;
			uint16_t Tile;
		};

		bool operator== (const TileAddress& other) const { return this->Address == other.Address; }
	};
	
	static constexpr TileAddress InvalidTileAddress = { uint32_t(-1) };
		

	struct TilePoolStats
	{
		uint32_t MaxTiles;
		uint32_t FreeTiles;

	};

	struct TilePage {
		uint32_t AllocateTile();
		void ReleaseTile(uint32_t tileNumber);

		uint32_t MaxTiles = 0;
		uint16_t PageIndex = 0;
		std::set<uint32_t> FreeTiles;
		TComPtr<ID3D12Heap> Heap;
	};

	class D3D12TilePool
	{
	public:
		void MapTexture(D3D12ResourceBatch& batch, 
			Ref<D3D12Texture2D> texture, 
			TComPtr<ID3D12CommandQueue> commandQueue);

		void ReleaseTexture(Ref<D3D12Texture2D> texture,
			TComPtr<ID3D12CommandQueue> commandQueue);

		std::vector<TilePoolStats> GetStats();

	private:
        struct TileAllocation
        {
            bool Mapped = false;
            D3D12_TILED_RESOURCE_COORDINATE ResourceCoordinate;
            TileAddress TileAddress;
        };

        struct MipAllocationInfo
        {
            std::vector<TileAllocation> TileAllocations;
        };

        struct TextureAllocationInfo
        {
            std::vector<MipAllocationInfo> MipAllocations;
            bool			PackedMipsMapped = false;
            TileAddress		PackedMipsAddress;

			~TextureAllocationInfo()
			{
			}
        };

        struct UpdateInfo
        {
            Ref<TilePage> Page = nullptr;

            std::vector<D3D12_TILED_RESOURCE_COORDINATE> startCoordinates;
            std::vector<D3D12_TILE_REGION_SIZE> regionSizes;
            std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags;
            std::vector<uint32_t> heapRangeStartOffsets;
            std::vector<uint32_t> rangeTileCounts;
            uint32_t tilesUpdated = 0;
        };

	private:

		/// <summary>
		/// Will search for a page that can fit the requested number of tiles. If
		/// no page was found, it returns nullptr
		/// </summary>
		/// <param name="tiles">The number of tiles that are required out of the page</param>
		/// <returns></returns>
		Ref<TilePage> FindAvailablePage(uint32_t tiles);

		/// <summary>
		/// Will add a new page with the requested size.
		/// </summary>
		/// <param name="batch">The resource batch that will be used to create this page</param>
		/// <param name="size">The size of the page in bytes</param>
		/// <returns>A reference to the newly added page</returns>
		Ref<TilePage> AddPage(D3D12ResourceBatch& batch, uint32_t size);

		Ref<TextureAllocationInfo> GetTextureInfo(Ref<D3D12VirtualTexture2D> texture);

		inline void ReleaseTile(TileAddress& address)
		{
			m_Pages[address.Page]->ReleaseTile(address.Tile);
		}
	
	private:
		uint32_t m_FrameCounter;

		std::unordered_map<Ref<D3D12VirtualTexture2D>, Ref<TextureAllocationInfo>> m_AllocationMap;
		std::vector<Ref<TilePage>> m_Pages;
	};
}

