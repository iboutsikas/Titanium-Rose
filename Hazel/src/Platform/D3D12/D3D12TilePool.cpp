#include "hzpch.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Hazel
{
    void D3D12TilePool::MapTexture(D3D12ResourceBatch& batch, Ref<D3D12Texture2D> texture, TComPtr<ID3D12CommandQueue> commandQueue)
    {
        if (!texture->IsVirtual())
        {
            return;
        }
        Ref<D3D12VirtualTexture2D> vTexture = std::dynamic_pointer_cast<D3D12VirtualTexture2D>(texture);

        auto search = m_AllocationMap.find(vTexture);
        Ref<TextureAllocationInfo> allocInfo = nullptr;

        if (search == m_AllocationMap.end())
        {
            // We do not know about this texture, lets add it
            allocInfo = CreateRef<TextureAllocationInfo>();
            m_AllocationMap[vTexture] = allocInfo;
            allocInfo->PackedMipsMapped = false;
            allocInfo->MipAllocations.resize(vTexture->m_MipInfo.NumStandardMips);
            for (uint32_t i = 0; i < allocInfo->MipAllocations.size(); i++)
            {
                auto& a = allocInfo->MipAllocations[i];
                auto& tiling = vTexture->m_Tilings[i];
                uint32_t size = tiling.WidthInTiles * tiling.HeightInTiles * tiling.DepthInTiles;
                a.TileAllocations.resize(size);
                for (auto& ma : a.TileAllocations)
                {
                    ma.Mapped = false;
                }
            }
        }
        else
        {
            allocInfo = search->second;
        }

        // These should stay mapped
        if (!allocInfo->PackedMipsMapped)
        {
            uint32_t packedSubresource = (vTexture->m_MipInfo.NumPackedMips > 0 ? vTexture->m_MipInfo.NumStandardMips : 0);
                

            Ref<TilePage> page = FindAvailablePage(1);
            if (page == nullptr)
            {
                page = this->AddPage(batch, 4096 * 2048 * 4);
            }
            HZ_CORE_ASSERT(page != nullptr, "Somehow we don't have any pages free");
            allocInfo->PackedMipsPage = page;
            allocInfo->PackedMipsOffset = page->PopTile();


            D3D12_TILED_RESOURCE_COORDINATE startCoordinate = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, packedSubresource);
            D3D12_TILE_REGION_SIZE regionSize = {};
            regionSize.NumTiles = vTexture->m_MipInfo.NumTilesForPackedMips;
            regionSize.UseBox = FALSE;

            HZ_CORE_ASSERT(regionSize.NumTiles == 1, "Currently we only supported packed mips into a single tile");

            D3D12_TILE_RANGE_FLAGS rangeFlag = D3D12_TILE_RANGE_FLAG_NONE;

            // NOTE: This assumes that all the tail mips are packed into one tile
            commandQueue->UpdateTileMappings(
                vTexture->GetResource(),
                1,
                &startCoordinate,
                &regionSize,
                page->Heap.Get(),
                1,
                &rangeFlag ,
                &(allocInfo->PackedMipsOffset),
                &(regionSize.NumTiles),
                D3D12_TILE_MAPPING_FLAG_NONE
            );
            
            allocInfo->PackedMipsMapped = true;
        }

        D3D12Texture2D::MipLevels mips = texture->GetMipsUsed();
        uint32_t invalidMip = texture->GetMipLevels();

        auto feedbackMap = texture->GetFeedbackMap();
        if (feedbackMap == nullptr) 
        {
            HZ_CORE_ASSERT(false, "Virtual texture with no feedback map cannot be allocated to tile pool");
        }
        auto dims = feedbackMap->GetDimensions();


        std::vector<D3D12_TILED_RESOURCE_COORDINATE> startCoordinates;
        /**
        typedef struct D3D12_TILE_REGION_SIZE
        {
            UINT NumTiles;
            BOOL UseBox;
            UINT Width;
            UINT16 Height;
            UINT16 Depth;
        } 	D3D12_TILE_REGION_SIZE;
        */
        std::vector<D3D12_TILE_REGION_SIZE> regionSizes;
        std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags;
        std::vector<uint32_t> heapRangeStartOffsets;
        std::vector<uint32_t> rangeTileCounts;
        uint32_t tilesUpdated = 0;

        uint32_t finalMappedMip = mips.FinestMip >= vTexture->m_MipInfo.NumStandardMips
            ? vTexture->m_MipInfo.NumStandardMips - 1
            : mips.FinestMip;

        // null map these
        for (uint32_t i = 0; i < finalMappedMip; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo->MipAllocations[i];

            if (mipAllocation.AllocatedPage == nullptr)
            {
                // This mip has not been mapped yet.
                continue;
            }

            auto& tileAllocations = mipAllocation.TileAllocations;

            for (uint32_t y = 0; y < yy; y++)
            {
                for (uint32_t x = 0; x < xx; x++)
                {
                    auto index = y * xx + x;
                    auto& tileAllocation = tileAllocations[index];
                    if (tileAllocation.Mapped) 
                    {
                        startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i));
                        regionSizes.push_back({ 1, TRUE, 1, 1, 1 });
                        rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NULL);
                        heapRangeStartOffsets.push_back(tileAllocation.HeapOffset);
                        rangeTileCounts.push_back(1);
                        mipAllocation.AllocatedPage->FreeTile(tileAllocation.HeapOffset);
                        tileAllocation.Mapped = false;
                        ++tilesUpdated;
                    }
                }
            }

            commandQueue->UpdateTileMappings(
                vTexture->GetResource(),
                tilesUpdated,
                startCoordinates.data(),
                regionSizes.data(),
                mipAllocation.AllocatedPage->Heap.Get(),
                tilesUpdated,
                rangeFlags.data(),
                heapRangeStartOffsets.data(),
                rangeTileCounts.data(),
                D3D12_TILE_MAPPING_FLAG_NONE
                );   
            startCoordinates.clear();
            regionSizes.clear();
            rangeFlags.clear();
            heapRangeStartOffsets.clear();
            rangeTileCounts.clear();
            tilesUpdated = 0;
            mipAllocation.AllocatedPage = nullptr;
        }

        

        finalMappedMip = mips.CoarsestMip >= vTexture->m_MipInfo.NumStandardMips 
            ? vTexture->m_MipInfo.NumStandardMips - 1 
            : mips.CoarsestMip;

        // map each of those mips to the pool
        for (uint32_t i = mips.FinestMip; i <= finalMappedMip; i++)
        {
            HZ_CORE_ASSERT(tilesUpdated == 0, "Not everything has been reset here, wtf ?");


            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo->MipAllocations[i];

            // In this case we need to find what page we are allocating for this mip
            if (mipAllocation.AllocatedPage == nullptr) {
                auto& tiling = vTexture->m_Tilings[i];
                uint32_t size = tiling.WidthInTiles * tiling.HeightInTiles * tiling.DepthInTiles;
                auto page = this->FindAvailablePage(size);
                if (page == nullptr) {
                    page = this->AddPage(batch, 4096 * 2048 * 4);
                }
                mipAllocation.AllocatedPage = page;
            }

            auto& tileAllocations = mipAllocation.TileAllocations;


            for (uint32_t y = 0; y < yy; y++)
            {
                for (uint32_t x = 0; x < xx; x++)
                {
                    auto index = y * xx + x;
                    auto& tileAllocation = tileAllocations[index];
                    // Is already mapped, let's skip
                    if (tileAllocation.Mapped)
                    {
                        continue;
                    }

                    startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i));
                    regionSizes.push_back({ 1, TRUE, 1, 1, 1 });
                    rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
                    tileAllocation.HeapOffset = mipAllocation.AllocatedPage->PopTile();

                    heapRangeStartOffsets.push_back(tileAllocation.HeapOffset);
                    rangeTileCounts.push_back(1);

                    //mipAllocation.AllocatedPage->FreeTile(tileAllocation.HeapOffset);
                    tileAllocation.Mapped = true;
                    ++tilesUpdated;

                }
            }

            // We should not call UpdateTileMappings
            if (tilesUpdated == 0)
            {
                continue;
            }


            commandQueue->UpdateTileMappings(
                vTexture->GetResource(),
                tilesUpdated,
                startCoordinates.data(),
                regionSizes.data(),
                mipAllocation.AllocatedPage->Heap.Get(),
                tilesUpdated,
                rangeFlags.data(),
                heapRangeStartOffsets.data(),
                rangeTileCounts.data(),
                D3D12_TILE_MAPPING_FLAG_NONE
            );
            startCoordinates.clear();
            regionSizes.clear();
            rangeFlags.clear();
            heapRangeStartOffsets.clear();
            rangeTileCounts.clear();
            tilesUpdated = 0;
        }

        for (uint32_t i = finalMappedMip + 1; i < vTexture->m_MipInfo.NumStandardMips; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo->MipAllocations[i];

            if (mipAllocation.AllocatedPage == nullptr)
            {
                // This mip has not been mapped yet.
                continue;
            }

            auto& tileAllocations = mipAllocation.TileAllocations;

            for (uint32_t y = 0; y < yy; y++)
            {
                for (uint32_t x = 0; x < xx; x++)
                {
                    auto index = y * xx + x;
                    auto& tileAllocation = tileAllocations[index];
                    if (tileAllocation.Mapped)
                    {
                        startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i));
                        regionSizes.push_back({ 1, TRUE, 1, 1, 1 });
                        rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NULL);
                        heapRangeStartOffsets.push_back(tileAllocation.HeapOffset);
                        rangeTileCounts.push_back(1);
                        mipAllocation.AllocatedPage->FreeTile(tileAllocation.HeapOffset);
                        tileAllocation.Mapped = false;
                        ++tilesUpdated;
                    }
                }
            }

            commandQueue->UpdateTileMappings(
                vTexture->GetResource(),
                tilesUpdated,
                startCoordinates.data(),
                regionSizes.data(),
                mipAllocation.AllocatedPage->Heap.Get(),
                tilesUpdated,
                rangeFlags.data(),
                heapRangeStartOffsets.data(),
                rangeTileCounts.data(),
                D3D12_TILE_MAPPING_FLAG_NONE
            );
            startCoordinates.clear();
            regionSizes.clear();
            rangeFlags.clear();
            heapRangeStartOffsets.clear();
            rangeTileCounts.clear();
            tilesUpdated = 0;
            mipAllocation.AllocatedPage = nullptr;
        }
#if 0

        auto dims = feedbackMap->GetDimensions();
        uint32_t* data = feedbackMap->GetData<uint32_t*>();
        uint32_t currentMip = 0;

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
                  
                    alloc.HeapOffset = m_Pages[0].PopTile();
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
            tilesUpdated,
            rangeFlags.data(),
            heapRangeStartOffsets.data(),
            rangeTileCounts.data(),
            D3D12_TILE_MAPPING_FLAG_NONE
        );
#endif
    }

    std::vector<TilePoolStats> D3D12TilePool::GetStats()
    {
        std::vector<TilePoolStats> ret;


        for (auto page : m_Pages)
        {
            TilePoolStats stats;
            stats.MaxTiles = page->MaxTiles;
            stats.FreeTiles = page->FreeTiles.size();
            ret.push_back(stats);
        }
        return ret;
    }

    Ref<TilePage> D3D12TilePool::FindAvailablePage(uint32_t tiles)
    {
        Ref<TilePage> ret = nullptr;
        for (auto p : m_Pages)
        {
            if (p->FreeTiles.size() >= tiles)
            {
                ret = p;
                break;
            }
        }
        return ret;
    }

    Ref<TilePage> D3D12TilePool::AddPage(D3D12ResourceBatch& batch, uint32_t size)
    {
        Ref<TilePage> newPage = CreateRef<TilePage>();

        D3D12_HEAP_DESC heapDesc = CD3DX12_HEAP_DESC(size, D3D12_HEAP_TYPE_DEFAULT);
        TComPtr<ID3D12Heap> heap = nullptr;

        D3D12::ThrowIfFailed(batch.GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

        uint32_t numTiles = size / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        newPage->Heap = heap;
        newPage->MaxTiles = numTiles;

        for (size_t i = 0; i < numTiles; i++)
        {
            newPage->FreeTiles.insert(i);
        }

        m_Pages.push_back(newPage);
        return newPage;
    }


    uint32_t TilePage::PopTile()
    {
        auto iter = this->FreeTiles.begin();
        auto tileNumber = *iter;
        this->FreeTiles.erase(iter);
        return tileNumber;
    }

    void TilePage::FreeTile(uint32_t tile)
    {
        HZ_CORE_ASSERT(tile < MaxTiles, "Tile is outside the available tile range");
        this->FreeTiles.insert(tile);
    }
}
