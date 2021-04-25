#include "hzpch.h"
#include "Platform/D3D12/D3D12TilePool.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Renderer.h"
#include "Platform/D3D12/CommandQueue.h"
#include "Platform/D3D12/D3D12FeedbackMap.h"

namespace Hazel
{
    //static constexpr TileAddress InvalidTileAddress = { uint32_t(-1) };

    void D3D12TilePool::MapTexture(VirtualTexture2D& texture)
    {
        if (!texture.IsVirtual())
        {
            HZ_CORE_ASSERT(false, "How did we get here with a non-virtual texture");
            return;
        }


        HZ_CORE_ASSERT(texture.m_MipInfo.NumTilesForPackedMips <= 1, "Currently we only support packed mips on 1 tile");

        TextureAllocationInfo& allocInfo = GetTextureInfo(texture);

        std::vector<UpdateInfo> updates(m_Pages.size());
        for (uint32_t u = 0; u < updates.size(); u++)
        {
            updates[u].Page = m_Pages[u];
        }
        Ref<TilePage> currentPage = FindAvailablePage(1);

        Texture2D::MipLevelsUsed mips = texture.GetMipsUsed();

        glm::ivec3 dims;
        auto feedbackMap = texture.GetFeedbackMap();
        if (feedbackMap == nullptr)
        {
            dims = texture.GetTileDimensions(0);
        }
        else
        {
            dims = feedbackMap->GetDimensions();
        }

        // =================== MAP PACKED MIPS ======================================
        if (!allocInfo.PackedMipsMapped && texture.m_MipInfo.NumTilesForPackedMips > 0)
        {
            uint32_t packedSubresource = (texture.m_MipInfo.NumPackedMips > 0 ? texture.m_MipInfo.NumStandardMips : 0);
                
            if (currentPage == nullptr) {
                currentPage = this->AddPage(4096 * 2048 * 4);
                UpdateInfo newInfo;
                newInfo.Page = currentPage;
                updates.push_back(newInfo);
            }

            allocInfo.PackedMipsAddress.Page = currentPage->PageIndex;
            allocInfo.PackedMipsAddress.Tile = currentPage->AllocateTile();

            auto& info = updates[currentPage->PageIndex];

            info.startCoordinates.push_back(CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, packedSubresource));
            info.regionSizes.push_back({ texture.m_MipInfo.NumTilesForPackedMips, FALSE, 1, 1, 1 });
            info.rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NONE);
            info.heapRangeStartOffsets.push_back(allocInfo.PackedMipsAddress.Tile);
            info.rangeTileCounts.push_back(1);
            ++info.tilesUpdated;

            allocInfo.PackedMipsMapped = true;
        }

        
        // =================== NULL MAP THE BEGINNING =================================
        uint32_t finalMappedMip = (mips.FinestMip >= texture.m_MipInfo.NumStandardMips && texture.m_MipInfo.NumStandardMips != 0)
            ? texture.m_MipInfo.NumStandardMips - 1
            : mips.FinestMip;

        for (uint32_t i = 0; i < finalMappedMip; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo.MipAllocations[i];


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
        finalMappedMip = (mips.CoarsestMip >= texture.m_MipInfo.NumStandardMips && texture.m_MipInfo.NumStandardMips != 0)
            ? texture.m_MipInfo.NumStandardMips
            : mips.CoarsestMip + 1;

        //finalMappedMip = vTexture->m_MipInfo.NumStandardMips;

        for (uint32_t i = mips.FinestMip; i < finalMappedMip; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo.MipAllocations[i];

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

                    if (currentPage->NumFreeTiles() == 0) {
                        currentPage = FindAvailablePage(1);
                        if (currentPage == nullptr) {
                            currentPage = this->AddPage(4096 * 2048 * 4);
                            UpdateInfo newInfo;
                            newInfo.Page = currentPage;
                            updates.push_back(newInfo);
                        }
                    }

                    auto& info = updates[currentPage->PageIndex];

                    tileAllocation.TileAddress.Page = currentPage->PageIndex;
                    tileAllocation.TileAddress.Tile = currentPage->AllocateTile();
                    if (tileAllocation.TileAddress.Page == 0 && tileAllocation.TileAddress.Tile == 0) {
                        __debugbreak();
                    }
                    
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
        for (uint32_t i = finalMappedMip + 1; i < texture.m_MipInfo.NumStandardMips; i++)
        {
            uint32_t xx = dims.x >> i;
            uint32_t yy = dims.y >> i;
            auto& mipAllocation = allocInfo.MipAllocations[i];

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
                        info.Page->ReleaseTile(tileAllocation.TileAddress.Tile);
                        tileAllocation.Mapped = false;
                        ++info.tilesUpdated;
                    }
                }
            }
        }
#endif
        // ======================== APPLY THE UPDATES ===================
        auto& commandQueue = D3D12Renderer::CommandQueueManager.GetGraphicsQueue();

        for (auto& info : updates)
        {
            if (info.tilesUpdated == 0)
                continue;

            commandQueue.GetRawPtr()->UpdateTileMappings(
                texture.GetResource(),
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

    void D3D12TilePool::ReleaseTexture(VirtualTexture2D& texture)
    {
        if (!texture.IsVirtual())
        {
            HZ_CORE_ASSERT(false, "How did we ended up with a non-virtual texture here?");
            return;
        }

        TextureAllocationInfo& allocInfo = GetTextureInfo(texture);

        std::vector<UpdateInfo> updates(m_Pages.size());
        for (uint32_t u = 0; u < updates.size(); u++)
        {
            updates[u].Page = m_Pages[u];
        }

        Texture2D::MipLevelsUsed mips = texture.GetMipsUsed();

        glm::ivec3 dims;
        auto feedbackMap = texture.GetFeedbackMap();
        if (feedbackMap == nullptr)
        {
            dims = texture.GetTileDimensions(0);
        }
        else
        {
            dims = feedbackMap->GetDimensions();
        }

        //======= Release Packed Mips =======

        if (allocInfo.PackedMipsMapped) {
            this->ReleaseTile(allocInfo.PackedMipsAddress);
        }

        for (auto& allocation : allocInfo.MipAllocations)
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
        auto commandQueue = D3D12Renderer::CommandQueueManager.GetGraphicsQueue();

        commandQueue.GetRawPtr()->UpdateTileMappings(
            texture.GetResource(),
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

        //this->m_AllocationMap.erase(vTexture);
    }

    void D3D12TilePool::RemoveTexture(VirtualTexture2D& texture)
    {
        m_AllocationMap.erase(&texture);
    }

    std::vector<TilePoolStats> D3D12TilePool::GetStats()
    {
        std::vector<TilePoolStats> ret;

        for (auto page : m_Pages)
        {
            TilePoolStats stats;
            stats.MaxTiles = page->Size();
            stats.FreeTiles = page->NumFreeTiles();
            ret.push_back(stats);
        }
        return ret;
    }

    uint64_t D3D12TilePool::GetTilesUsed(VirtualTexture2D& texture)
    {
        TextureAllocationInfo& textureInfo = this->GetTextureInfo(texture);
        uint64_t tilesUsed = 0;

        for (auto& mip : textureInfo.MipAllocations) {
            for (auto tile : mip.TileAllocations) {
                if (tile.Mapped) {
                    tilesUsed++;
                }
            }
        }

        if (textureInfo.PackedMipsMapped) {
            tilesUsed++;
        }

        return tilesUsed;
    }

    Ref<TilePage> D3D12TilePool::FindAvailablePage(uint32_t tiles)
    {
        Ref<TilePage> ret = nullptr;
        for (auto p : m_Pages)
        {
            if (p->NumFreeTiles() >= tiles)
            {
                ret = p;
                break;
            }
        }
        return ret;
    }

    Ref<TilePage> D3D12TilePool::AddPage(uint32_t size)
    {

        D3D12_HEAP_DESC heapDesc = CD3DX12_HEAP_DESC(size, D3D12_HEAP_TYPE_DEFAULT);
        TComPtr<ID3D12Heap> heap = nullptr;

        D3D12::ThrowIfFailed(D3D12Renderer::GetDevice()->CreateHeap(
            &heapDesc, 
            IID_PPV_ARGS(&heap)
        ));

        uint32_t numTiles = size / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        Ref<TilePage> newPage = CreateRef<TilePage>(numTiles, m_Pages.size());
        newPage->PageIndex = m_Pages.size();
        newPage->Heap = heap;
        m_Pages.push_back(newPage);
        return newPage;
    }

    D3D12TilePool::TextureAllocationInfo& D3D12TilePool::GetTextureInfo(VirtualTexture2D& texture)
    {
        auto search = m_AllocationMap.find(&texture);
        TextureAllocationInfo allocInfo;

        if (search != m_AllocationMap.end())
            return search->second;

        // We do not know about this texture, lets add it
        allocInfo.PackedMipsMapped = false;
        allocInfo.MipAllocations.resize(texture.m_MipInfo.NumStandardMips);
        for (uint32_t i = 0; i < allocInfo.MipAllocations.size(); i++)
        {
            auto& a = allocInfo.MipAllocations[i];
            auto& tiling = texture.m_Tilings[i];
            uint32_t size = tiling.WidthInTiles * tiling.HeightInTiles * tiling.DepthInTiles;
            a.TileAllocations.resize(size);
            for (auto&&  ma : a.TileAllocations)
            {
                ma.Mapped = false;
            }
        }
        m_AllocationMap[&texture] = allocInfo;
      
        return m_AllocationMap[&texture];
    }

    uint32_t TilePage::AllocateTile()
    {
        HZ_CORE_ASSERT(m_FreeTiles > 0, "There are no free tiles on this page");

        for (uint32_t i = 0; i < m_MaxTiles; i++)
        {
            if (FreeTiles2[i] == 1)
                continue;

            FreeTiles2[i] = 1;
            --m_FreeTiles;

            return i;
        }
        HZ_CORE_ASSERT(false, "Should never reach this");
        return -1;
    }

    void TilePage::ReleaseTile(uint32_t tile)
    {
        HZ_CORE_ASSERT(tile < m_MaxTiles, "Tile is outside the available tile range");
        if (FreeTiles2[tile] == 0) {
            //HZ_CORE_ASSERT(false, "Trying to free an already freed tile");
            return;
        }
        FreeTiles2[tile] = 0;
        ++m_FreeTiles;
    }
}
