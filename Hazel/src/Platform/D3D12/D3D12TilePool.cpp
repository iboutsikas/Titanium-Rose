#include "hzpch.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Renderer.h"

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

        HZ_CORE_ASSERT(vTexture->m_MipInfo.NumTilesForPackedMips <= 1, "Currently we only support packed mips on 1 tile");

        Ref<TextureAllocationInfo> allocInfo = GetTextureInfo(vTexture);

        std::vector<UpdateInfo> updates(m_Pages.size());
        for (uint32_t u = 0; u < updates.size(); u++)
        {
            updates[u].Page = m_Pages[u];
        }
        Ref<TilePage> currentPage = FindAvailablePage(1);

        D3D12Texture2D::MipLevels mips = texture->GetMipsUsed();

        glm::ivec3 dims;
        auto feedbackMap = texture->GetFeedbackMap();
        if (feedbackMap == nullptr)
        {
            dims = vTexture->GetTileDimensions(0);
        }
        else
        {
            dims = feedbackMap->GetDimensions();
        }

        // =================== MAP PACKED MIPS ======================================
        if (!allocInfo->PackedMipsMapped && vTexture->m_MipInfo.NumTilesForPackedMips > 0)
        {
            uint32_t packedSubresource = (vTexture->m_MipInfo.NumPackedMips > 0 ? vTexture->m_MipInfo.NumStandardMips : 0);
                
            if (currentPage == nullptr) {
                currentPage = this->AddPage(batch, 4096 * 2048 * 4);
                UpdateInfo newInfo;
                newInfo.Page = currentPage;
                updates.push_back(newInfo);
            }

            allocInfo->PackedMipsAddress.Page = currentPage->PageIndex;
            allocInfo->PackedMipsAddress.Tile = currentPage->AllocateTile();

            auto& info = updates[currentPage->PageIndex];

            info.startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, packedSubresource));
            info.regionSizes.push_back({ vTexture->m_MipInfo.NumTilesForPackedMips, FALSE, 1, 1, 1 });
            info.rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
            info.heapRangeStartOffsets.push_back(allocInfo->PackedMipsAddress.Tile);
            info.rangeTileCounts.push_back(1);
            ++info.tilesUpdated;

            allocInfo->PackedMipsMapped = true;
        }

        
        // =================== NULL MAP THE BEGINNING =================================
        uint32_t finalMappedMip = (mips.FinestMip >= vTexture->m_MipInfo.NumStandardMips && vTexture->m_MipInfo.NumStandardMips != 0)
            ? vTexture->m_MipInfo.NumStandardMips - 1
            : mips.FinestMip;

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
        }

        
        // =================== MAP THE USED MIPS TO THE POOL ==========================
        finalMappedMip = (mips.CoarsestMip >= vTexture->m_MipInfo.NumStandardMips && vTexture->m_MipInfo.NumStandardMips != 0)
            ? vTexture->m_MipInfo.NumStandardMips 
            : mips.CoarsestMip + 1;

        //finalMappedMip = vTexture->m_MipInfo.NumStandardMips;

        for (uint32_t i = mips.FinestMip; i < finalMappedMip; i++)
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
        }

        // =============== NULL MAP THE ONES BETWEEN USED AND PACKED ==================
#if 1
        for (uint32_t i = finalMappedMip + 1; i < vTexture->m_MipInfo.NumStandardMips; i++)
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
        }
#endif
        // ======================== APPLY THE UPDATES ===================
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
    }

    void D3D12TilePool::ReleaseTexture(Ref<D3D12Texture2D> texture, TComPtr<ID3D12CommandQueue> commandQueue)
    {
        if (!texture->IsVirtual())
        {
            return;
        }

        Ref<D3D12VirtualTexture2D> vTexture = std::dynamic_pointer_cast<D3D12VirtualTexture2D>(texture);

        Ref<TextureAllocationInfo> allocInfo = GetTextureInfo(vTexture);

        std::vector<UpdateInfo> updates(m_Pages.size());
        for (uint32_t u = 0; u < updates.size(); u++)
        {
            updates[u].Page = m_Pages[u];
        }

        D3D12Texture2D::MipLevels mips = texture->GetMipsUsed();

        glm::ivec3 dims;
        auto feedbackMap = texture->GetFeedbackMap();
        if (feedbackMap == nullptr)
        {
            dims = vTexture->GetTileDimensions(0);
        }
        else
        {
            dims = feedbackMap->GetDimensions();
        }

        //======= Release Packed Mips =======

        if (allocInfo->PackedMipsMapped) {
            this->ReleaseTile(allocInfo->PackedMipsAddress);
        }

        for (auto& allocation : allocInfo->MipAllocations)
        {
            for (auto& tile : allocation.TileAllocations)
            {
                if (!tile.Mapped)
                    continue;

                this->ReleaseTile(tile.TileAddress);
                tile.Mapped = false;
            }
        }

        D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NULL;

        commandQueue->UpdateTileMappings(
            texture->GetResource(),
            1,
            nullptr,
            nullptr,
            nullptr,
            1,
            &rangeFlags,
            nullptr,
            nullptr,
            D3D12_TILE_MAPPING_FLAG_NONE
        );

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

    uint64_t D3D12TilePool::GetTilesUsed(Ref<D3D12VirtualTexture2D> texture)
    {
        Ref<TextureAllocationInfo> textureInfo = this->GetTextureInfo(texture);
        uint64_t tilesUsed = 0;

        for (auto mip : textureInfo->MipAllocations) {
            for (auto tile : mip.TileAllocations) {
                if (tile.Mapped) {
                    tilesUsed++;
                }
            }
        }

        if (textureInfo->PackedMipsMapped) {
            tilesUsed++;
        }

        return tilesUsed;
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

    Ref<D3D12TilePool::TextureAllocationInfo> D3D12TilePool::GetTextureInfo(Ref<D3D12VirtualTexture2D> texture)
    {
        auto search = m_AllocationMap.find(texture);
        Ref<TextureAllocationInfo> allocInfo = nullptr;

        if (search == m_AllocationMap.end())
        {
            // We do not know about this texture, lets add it
            allocInfo = CreateRef<TextureAllocationInfo>();
            m_AllocationMap[texture] = allocInfo;
            allocInfo->PackedMipsMapped = false;
            allocInfo->MipAllocations.resize(texture->m_MipInfo.NumStandardMips);
            for (uint32_t i = 0; i < allocInfo->MipAllocations.size(); i++)
            {
                auto& a = allocInfo->MipAllocations[i];
                auto& tiling = texture->m_Tilings[i];
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

        return allocInfo;
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
