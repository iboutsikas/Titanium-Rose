#include "hzpch.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Hazel
{
    void D3D12TilePool::MapTexture(D3D12ResourceUploadBatch& batch, Ref<D3D12Texture2D> texture, TComPtr<ID3D12CommandQueue> commandQueue)
    {
        if (!texture->IsVirtual())
        {
            return;
        }
        Ref<D3D12VirtualTexture2D> vTexture = std::dynamic_pointer_cast<D3D12VirtualTexture2D>(texture);

        if (m_Pages.empty())
        {
            D3D12_HEAP_DESC heapDesc = CD3DX12_HEAP_DESC(4096 * 2048 * 4, D3D12_HEAP_TYPE_DEFAULT);
            TComPtr<ID3D12Heap> heap = nullptr;

            D3D12::ThrowIfFailed(batch.GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

            uint32_t numTiles = (4096 * 2048 * 4) / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

            m_Pages.emplace_back();
            m_Pages[0].Heap = heap;

            for (size_t i = 0; i < numTiles; i++)
            {
                m_Pages[0].FreeTiles.insert(i);
            }
        }

        uint32_t invalidMip = texture->GetMipLevels();

        auto feedbackMap = texture->GetFeedbackMap();
        if (feedbackMap == nullptr) 
        {
            HZ_CORE_ASSERT(false, "Virtual texture with no feedback map cannot be allocated to tile pool");
        }
        auto dims = feedbackMap->GetDimensions();
        uint32_t* data = feedbackMap->GetData<uint32_t*>();

        uint32_t tilesUpdated = 0;
        for (int y = 0; y < dims.y; ++y) 
        {
            for (int x = 0; x < dims.x; ++x)
            {
                uint32_t index = y * dims.x + x;
                auto& alloc = vTexture->m_TileAllocations[0][index];

                if (data[index] == invalidMip)
                {
                    alloc.Mapped = false;
                    m_Pages[0].FreeTiles.insert(alloc.HeapOffset);
                    ++tilesUpdated;
                }
                else
                {
                    if (alloc.Mapped)
                    {
                        // It is already mapped, leave it be
                        continue;
                    }

                    // We need to map this
                    auto iter = m_Pages[0].FreeTiles.begin();
                    auto tileNumber = *iter;
                    m_Pages[0].FreeTiles.erase(iter);
                    alloc.HeapOffset = tileNumber;
                    alloc.Mapped = true;
                    ++tilesUpdated;
                }
            

            }
        }

        if (tilesUpdated == 0) {
            return;
        }

        // Now we need to create the mappings
        std::vector<D3D12_TILED_RESOURCE_COORDINATE> startCoordinates;
        std::vector<D3D12_TILE_REGION_SIZE> regionSizes;
        std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags;
        std::vector<uint32_t> heapRangeStartOffsets;
        std::vector<uint32_t> rangeTileCounts;

        for (size_t i = 0; i < vTexture->m_TileAllocations[0].size(); i++)
        {
            auto& alloc = vTexture->m_TileAllocations[0][i];

            startCoordinates.push_back(alloc.ResourceCoordinate);
            // Each region is 1x1x1 now. Maybe we could concatenate them
            regionSizes.push_back({ 1, TRUE, 1, 1, 1 });

            if (alloc.Mapped) {
                rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
            }
            else {
                rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NULL);
            }

            heapRangeStartOffsets.push_back(alloc.HeapOffset);
            // Again every region has 1 tile only;
            rangeTileCounts.push_back(1);
        }

        commandQueue->UpdateTileMappings(
            vTexture->GetResource(),
            tilesUpdated,
            startCoordinates.data(),
            regionSizes.data(),
            m_Pages[0].Heap.Get(),
            vTexture->m_TileAllocations[0].size(),
            rangeFlags.data(),
            heapRangeStartOffsets.data(),
            rangeTileCounts.data(),
            D3D12_TILE_MAPPING_FLAG_NONE
        );

    }
}
