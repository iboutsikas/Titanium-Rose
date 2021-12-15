#pragma once

#include <set>

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"

namespace std {
    
}


namespace Roses 
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

		TilePage(uint32_t maxTiles, uint16_t pageIndex)
			: FreeTiles2(nullptr), 
			m_MaxTiles(maxTiles), 
			PageIndex(pageIndex),
			m_FreeTiles(maxTiles)
		{
			FreeTiles2 = new uint8_t[maxTiles];
			memset(FreeTiles2, 0, maxTiles);
		}

		~TilePage()
		{
			delete[] FreeTiles2;
		}

		uint32_t AllocateTile();
		inline uint32_t NumFreeTiles() const { return m_FreeTiles; }
		inline uint32_t Size() const { return m_MaxTiles; }
		void ReleaseTile(uint32_t tileNumber);

		uint16_t PageIndex = 0;
		//std::set<uint32_t> FreeTiles;
		TComPtr<ID3D12Heap> Heap;
	private:
		uint8_t* FreeTiles2;
		uint32_t m_MaxTiles;
		uint32_t m_FreeTiles;
	};

	class D3D12TilePool
	{
	public:
		void MapTexture(VirtualTexture2D& texture);
		void ReleaseTexture(VirtualTexture2D& texture);
		void RemoveTexture(VirtualTexture2D& texture);
        uint64_t GetTilesUsed(VirtualTexture2D& texture);


		inline void MapTexture(Texture2D& texture) {
			MapTexture(MakeVirtualTexture(texture));
		}

		inline void ReleaseTexture(Texture2D& texture) {
			ReleaseTexture(MakeVirtualTexture(texture));
		}

		inline void RemoveTexture(Texture2D& texture) {
			RemoveTexture(MakeVirtualTexture(texture));
		}		

		inline uint64_t GetTilesUsed(Texture2D& texture) {
			return GetTilesUsed(MakeVirtualTexture(texture));
		}

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

        struct CustomKeyHash {
            std::size_t operator()(const Roses::Ref<Roses::VirtualTexture2D> k) const {
                return std::hash<Roses::VirtualTexture2D*>()(k.get());
            }
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
		Ref<TilePage> AddPage(uint32_t size);

        TextureAllocationInfo& GetTextureInfo(VirtualTexture2D& texture);

		inline void ReleaseTile(TileAddress& address)
		{
			m_Pages[address.Page]->ReleaseTile(address.Tile);
		}
		
		inline VirtualTexture2D& MakeVirtualTexture(Texture2D& texture) 
		{
			HZ_CORE_ASSERT(texture.IsVirtual(), "How did we get here with a non-virtual texture");
			return dynamic_cast<VirtualTexture2D&>(texture);
		}
	private:
		uint32_t m_FrameCounter;

		std::unordered_map<VirtualTexture2D*, TextureAllocationInfo> m_AllocationMap;
		std::vector<Ref<TilePage>> m_Pages;
	};
}

