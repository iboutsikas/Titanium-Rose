#include "hzpch.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/D3D12Helpers.h"

namespace Hazel
{
    //static constexpr TileAddress InvalidTileAddress = { uint32_t(-1) };

    void D3D12TilePool::MapTexture(D3D12ResourceBatch& batch, Ref<D3D12Texture2D> texture, TComPtr<ID3D12CommandQueue> commandQueue)
    {
        if (!texture->IsVirtual())
        {
            return;
        }

        Ref<D3D12VirtualTexture2D> vTexture = std::dynamic_pointer_cast<D3D12VirtualTexture2D>(texture);

        HZ_CORE_ASSERT(vTexture->m_MipInfo.NumTilesForPackedMips == 1, "Currently we only support packed mips on 1 tile");

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

        std::vector<UpdateInfo> updates(m_Pages.size());
        for (uint32_t u = 0; u < updates.size(); u++)
        {
            updates[u].Page = m_Pages[u];
        }
        Ref<TilePage> currentPage = FindAvailablePage(1);

        // These should stay mapped
        if (!allocInfo->PackedMipsMapped)
        {
            uint32_t packedSubresource = (vTexture->m_MipInfo.NumPackedMips > 0 ? vTexture->m_MipInfo.NumStandardMips : 0);
                
            if (currentPage == nullptr) {
                currentPage = this->AddPage(batch, 4096 * 2048 * 4);
                UpdateInfo newInfo;
                newInfo.Page = currentPage;
                updates.push_back(newInfo);
            }

            allocInfo->packedMipsAddress.Page = currentPage->PageIndex;
            allocInfo->packedMipsAddress.Tile = currentPage->AllocateTile();

            auto& info = updates[currentPage->PageIndex];

            info.startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, packedSubresource));
            info.regionSizes.push_back({ vTexture->m_MipInfo.NumTilesForPackedMips, FALSE, 1, 1, 1 });
            info.rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
            info.heapRangeStartOffsets.push_back(allocInfo->packedMipsAddress.Tile);
            info.rangeTileCounts.push_back(1);
            ++info.tilesUpdated;

            allocInfo->PackedMipsMapped = true;
        }

        D3D12Texture2D::MipLevels mips = texture->GetMipsUsed();

        auto feedbackMap = texture->GetFeedbackMap();
        if (feedbackMap == nullptr) 
        {
            HZ_CORE_ASSERT(false, "Virtual texture with no feedback map cannot be allocated to tile pool");
        }
        auto dims = feedbackMap->GetDimensions();

        uint32_t finalMappedMip = mips.FinestMip >= vTexture->m_MipInfo.NumStandardMips
            ? vTexture->m_MipInfo.NumStandardMips - 1
            : mips.FinestMip;


        // =================== NULL MAP THE BEGINNING =================================
        for (uint32_t i = 0; i < finalMappedMip; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo->MipAllocations[i];


            auto& tileAllocations = mipAllocation.TileAllocations;

            for (uint32_t y = 0; y < yy; y++)
            {
                for (uint32_t x = 0; x < xx; x++)
                {
                    auto index = y * xx + x;
                    auto& tileAllocation = tileAllocations[index];
                    if (tileAllocation.Mapped) 
                    {
                        auto& info = updates[tileAllocation.TileAddress.Page];

                        if (info.Page == nullptr) {
                            info.Page = m_Pages[tileAllocation.TileAddress.Page];
                        }


                        info.startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i));
                        info.regionSizes.push_back({ 1, TRUE, 1, 1, 1 });
                        info.rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NULL);
                        info.heapRangeStartOffsets.push_back(tileAllocation.TileAddress.Tile);
                        info.rangeTileCounts.push_back(1);
                        info.Page->ReleaseTile(tileAllocation.TileAddress.Tile);
                        tileAllocation.Mapped = false;
                        ++info.tilesUpdated;
                    }
                }
            }

            //for (auto& info : updates) 
            //{
            //    if (info.tilesUpdated == 0)
            //        continue;

            //    commandQueue->UpdateTileMappings(
            //        vTexture->GetResource(),
            //        info.tilesUpdated,
            //        info.startCoordinates.data(),
            //        info.regionSizes.data(),
            //        info.Page->Heap.Get(),
            //        info.tilesUpdated,
            //        info.rangeFlags.data(),
            //        info.heapRangeStartOffsets.data(),
            //        info.rangeTileCounts.data(),
            //        D3D12_TILE_MAPPING_FLAG_NONE
            //    );
            //}

            //commandQueue->UpdateTileMappings(
            //    vTexture->GetResource(),
            //    tilesUpdated,
            //    startCoordinates.data(),
            //    regionSizes.data(),
            //    mipAllocation.AllocatedPage->Heap.Get(),
            //    tilesUpdated,
            //    rangeFlags.data(),
            //    heapRangeStartOffsets.data(),
            //    rangeTileCounts.data(),
            //    D3D12_TILE_MAPPING_FLAG_NONE
            //);   

            //startCoordinates.clear();
            //regionSizes.clear();
            //rangeFlags.clear();
            //heapRangeStartOffsets.clear();
            //rangeTileCounts.clear();
            //tilesUpdated = 0;
            //mipAllocation.AllocatedPage = nullptr;
        }

        finalMappedMip = mips.CoarsestMip >= vTexture->m_MipInfo.NumStandardMips 
            ? vTexture->m_MipInfo.NumStandardMips - 1 
            : mips.CoarsestMip;

        // =================== MAP THE USED MIPS TO THE POOL ==========================
        for (uint32_t i = mips.FinestMip; i <= finalMappedMip; i++)
        {            
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo->MipAllocations[i];

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

                    if (currentPage->FreeTiles.size() == 0) {
                        currentPage = FindAvailablePage(1);
                        if (currentPage == nullptr) {
                            currentPage = this->AddPage(batch, 4096 * 2048 * 4);
                            UpdateInfo newInfo;
                            newInfo.Page = currentPage;
                            updates.push_back(newInfo);
                        }
                    }

                    auto& info = updates[currentPage->PageIndex];

                    tileAllocation.TileAddress.Page = currentPage->PageIndex;
                    tileAllocation.TileAddress.Tile = currentPage->AllocateTile();
                    
                    info.startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i));
                    info.regionSizes.push_back({ 1, TRUE, 1, 1, 1 });
                    info.rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
                    info.heapRangeStartOffsets.push_back(tileAllocation.TileAddress.Tile);
                    info.rangeTileCounts.push_back(1);

                    //mipAllocation.AllocatedPage->FreeTile(tileAllocation.HeapOffset);
                    tileAllocation.Mapped = true;
                    ++info.tilesUpdated;

                }
            }

            // We should not call UpdateTileMappings
            //if (tilesUpdated == 0)
            //{
            //    continue;
            //}


            //commandQueue->UpdateTileMappings(
            //    vTexture->GetResource(),
            //    tilesUpdated,
            //    startCoordinates.data(),
            //    regionSizes.data(),
            //    mipAllocation.AllocatedPage->Heap.Get(),
            //    tilesUpdated,
            //    rangeFlags.data(),
            //    heapRangeStartOffsets.data(),
            //    rangeTileCounts.data(),
            //    D3D12_TILE_MAPPING_FLAG_NONE
            //);
            //startCoordinates.clear();
            //regionSizes.clear();
            //rangeFlags.clear();
            //heapRangeStartOffsets.clear();
            //rangeTileCounts.clear();
            //tilesUpdated = 0;
        }

        for (uint32_t i = finalMappedMip + 1; i < vTexture->m_MipInfo.NumStandardMips; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo->MipAllocations[i];

            //if (mipAllocation.AllocatedPage == nullptr)
            //{
            //    // This mip has not been mapped yet.
            //    continue;
            //}

            auto& tileAllocations = mipAllocation.TileAllocations;

            for (uint32_t y = 0; y < yy; y++)
            {
                for (uint32_t x = 0; x < xx; x++)
                {
                    auto index = y * xx + x;
                    auto& tileAllocation = tileAllocations[index];

                    if (tileAllocation.Mapped)
                    {
                        auto& info = updates[tileAllocation.TileAddress.Page];

                        info.startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(x, y, 0, i));
                        info.regionSizes.push_back({ 1, TRUE, 1, 1, 1 });
                        info.rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NULL);
                        info.heapRangeStartOffsets.push_back(tileAllocation.TileAddress.Tile);
                        info.rangeTileCounts.push_back(1);
                        info.Page->ReleaseTile(tileAllocation.TileAddress.Page);
                        tileAllocation.Mapped = false;
                        ++info.tilesUpdated;
                    }
                }
            }

            //commandQueue->UpdateTileMappings(
            //    vTexture->GetResource(),
            //    tilesUpdated,
            //    startCoordinates.data(),
            //    regionSizes.data(),
            //    mipAllocation.AllocatedPage->Heap.Get(),
            //    tilesUpdated,
            //    rangeFlags.data(),
            //    heapRangeStartOffsets.data(),
            //    rangeTileCounts.data(),
            //    D3D12_TILE_MAPPING_FLAG_NONE
            //);
            //startCoordinates.clear();
            //regionSizes.clear();
            //rangeFlags.clear();
            //heapRangeStartOffsets.clear();
            //rangeTileCounts.clear();
            //tilesUpdated = 0;
            //mipAllocation.AllocatedPage = nullptr;
        }

        for (auto& info : updates)
        {
            if (info.tilesUpdated == 0)
                continue;

            commandQueue->UpdateTileMappings(
                vTexture->GetResource(),
                info.tilesUpdated,
                info.startCoordinates.data(),
                info.regionSizes.data(),
                info.Page->Heap.Get(),
                info.tilesUpdated,
                info.rangeFlags.data(),
                info.heapRangeStartOffsets.data(),
                info.rangeTileCounts.data(),
                D3D12_TILE_MAPPING_FLAG_NONE
            );
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
        newPage->PageIndex = m_Pages.size();

        m_Pages.push_back(newPage);
        return newPage;
    }


    uint32_t TilePage::AllocateTile()
    {
        auto iter = this->FreeTiles.begin();
        auto tileNumber = *iter;
        this->FreeTiles.erase(iter);
        return tileNumber;
    }

    void TilePage::ReleaseTile(uint32_t tile)
    {
        HZ_CORE_ASSERT(tile < MaxTiles, "Tile is outside the available tile range");
        this->FreeTiles.insert(tile);
    }
}
