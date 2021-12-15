#pragma once
#include <vector>
#include <queue>

#include "Platform/D3D12/GpuResource.h"

namespace Roses {

    struct DynamicAllocation {
        DynamicAllocation(GpuResource& resource, size_t size, size_t offset)
            : Buffer(resource), Offset(offset), Size(size)
        {}

        GpuResource& Buffer;
        size_t Offset;
        size_t Size;
        void* CpuAddress;
        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;

    };

    class LinearAllocator
    {

    public:

        enum AllocatorType {
            GpuExclusive,
            CpuWritable,
            Count
        };
    private:

        class LinearAllocatorPage: public GpuResource {
        public:
            LinearAllocatorPage(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) :
                GpuResource("", state)
            {
                m_Resource.Attach(resource);
                m_GpuVirtualAddress = m_Resource->GetGPUVirtualAddress();
                m_Resource->Map(0, nullptr, &m_CpuVirtualAddress);
            }

            ~LinearAllocatorPage() {

            }

            void Map() {
                if (m_CpuVirtualAddress != nullptr)
                    return;
                
                m_Resource->Map(0, nullptr, &m_CpuVirtualAddress);
            }

            void Unmap() {
                if (m_CpuVirtualAddress == nullptr)
                    return;

                m_Resource->Unmap(0, nullptr);
                m_CpuVirtualAddress = nullptr;
            }

            void* m_CpuVirtualAddress;
            // We need to expose this to the allocator, as the one in GpuResource
            // is private
            D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
        };

        class LinearAllocatorPageManager {
        public:
            LinearAllocatorPageManager(AllocatorType type);

            LinearAllocatorPage* RequestPage();
            LinearAllocatorPage* CreateNewPage(size_t size = 0);

            void DiscardPages(uint64_t fenceValue, const std::vector<LinearAllocatorPage*>& pages);

            void FreeLargePages(uint64_t fenceValue, const std::vector<LinearAllocatorPage*>& pages);

            void Destroy();

        private:


            AllocatorType m_Type;

            struct RetiredPage {
                uint64_t FenceValue;
                LinearAllocatorPage* Page;
            };

            struct DeletedPage {
                uint64_t FenceValue;
                LinearAllocatorPage* Page;
            };

            std::vector<std::unique_ptr<LinearAllocatorPage> > m_PagePool;
            std::queue<RetiredPage> m_RetiredPages;
            std::queue<DeletedPage> m_DeletionQueue;
            std::queue<LinearAllocatorPage*> m_AvailablePages;
        };

        enum AllocatorPageSize {
            CPU = 0x010000,     // 64k
            GPU = 0x200000      // 2MB
        };

    public:

        LinearAllocator(AllocatorType type);

        DynamicAllocation Allocate(size_t size, size_t alignment = 256);

        void CleanupUsedPages(uint64_t fenceValue);
    private:

        DynamicAllocation AllocateLargePage(size_t sizeInBytes);


        AllocatorType m_AllocationType;
        size_t m_PageSize;
        size_t m_Offset;
        LinearAllocatorPage* m_CurrentPage;

        static LinearAllocatorPageManager s_PageManagers[AllocatorType::Count];

        std::vector<LinearAllocatorPage*> m_RetiredPages;
        std::vector<LinearAllocatorPage*> m_LargePageList;
    };
}

