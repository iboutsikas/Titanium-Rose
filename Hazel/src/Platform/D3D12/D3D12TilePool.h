#pragma once

#include <set>

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"

namespace Hazel 
{
	struct TilePage {
		uint32_t PopTile();
		void FreeTile(uint32_t tileNumber);

		uint32_t MaxTiles = 0;
		std::set<uint32_t> FreeTiles;
		TComPtr<ID3D12Heap> Heap;
	};

	class D3D12TilePool
	{
	public:
		void MapTexture(D3D12ResourceBatch& batch, 
			Ref<D3D12Texture2D> texture, 
			TComPtr<ID3D12CommandQueue> commandQueue);
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

		//Ref<D3D12TilePool::TextureAllocationInfo> CreateInitialAllocation()

		struct TileAllocation
		{
			bool Mapped = false;
			uint32_t HeapOffset = 0;
			D3D12_TILED_RESOURCE_COORDINATE ResourceCoordinate;
		};

		struct MipAllocationInfo
		{
			uint32_t AllocatedTiles = 0;
			uint32_t NewTiles = 0;
			Ref<TilePage> AllocatedPage;
			std::vector<TileAllocation> TileAllocations;
		};
		
		struct TextureAllocationInfo
		{
			std::vector<MipAllocationInfo> MipAllocations;
			bool			PackedMipsMapped = false;
			uint32_t		PackedMipsOffset = 0;
			Ref<TilePage>	PackedMipsPage = nullptr;
		};

		std::unordered_map<Ref<D3D12VirtualTexture2D>, Ref<TextureAllocationInfo>> m_AllocationMap;

		std::vector<Ref<TilePage>> m_Pages;
	};
}

