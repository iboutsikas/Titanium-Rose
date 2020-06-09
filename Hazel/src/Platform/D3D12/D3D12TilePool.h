#pragma once

#include <set>

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12UploadBatch.h"

namespace Hazel 
{
	class D3D12TilePool
	{
	public:
		void MapTexture(D3D12ResourceUploadBatch& batch, Ref<D3D12Texture2D> texture, TComPtr<ID3D12CommandQueue> commandQueue);
	private:
		struct Tile {
			uint32_t x;
			uint32_t y;
		};

		struct Page {
			std::set<uint32_t> FreeTiles;
			TComPtr<ID3D12Heap> Heap;
		};		

		std::vector<Page> m_Pages;
	};
}

