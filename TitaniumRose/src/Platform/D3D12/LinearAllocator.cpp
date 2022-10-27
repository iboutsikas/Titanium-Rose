#include "trpch.h"
#include "Platform/D3D12/CommandQueue.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Renderer.h"

#include "LinearAllocator.h"

namespace Roses {

    LinearAllocator::LinearAllocatorPageManager LinearAllocator::s_PageManagers[] = {
        { AllocatorType::GpuExclusive },
        { AllocatorType::CpuWritable }
    };

    LinearAllocator::LinearAllocator(AllocatorType type):
        m_AllocationType(type), m_PageSize(0), 
        m_Offset(-1), m_CurrentPage(nullptr)
    {
        m_PageSize = (type == AllocatorType::GpuExclusive) ? 
            AllocatorPageSize::GPU : 
            AllocatorPageSize::CPU;
    }

    DynamicAllocation LinearAllocator::Allocate(size_t size, size_t alignment)
    {
        size_t alignmentMask = alignment - 1;

        HZ_CORE_ASSERT((alignmentMask & alignment) == 0, "Alignment not a power of 2");
        size_t actualSize = D3D12::AlignUpMasked(size, alignmentMask);

        
        if (actualSize > m_PageSize)
            return AllocateLargePage(actualSize);

        m_Offset = D3D12::AlignUp(m_Offset, alignment);

        if (m_Offset + actualSize > m_PageSize) {
            HZ_CORE_ASSERT(m_CurrentPage != nullptr, "We've got an offset but no page?!");
            m_RetiredPages.push_back(m_CurrentPage);
            m_CurrentPage = nullptr;
        }

        if (m_CurrentPage == nullptr) {
            m_CurrentPage = s_PageManagers[m_AllocationType].RequestPage();
            m_Offset = 0;
        }

        DynamicAllocation alloc(*m_CurrentPage, actualSize, m_Offset);
        alloc.CpuAddress = (uint8_t*)m_CurrentPage->m_CpuVirtualAddress + m_Offset;
        alloc.GpuAddress = m_CurrentPage->GetGPUAddress() + m_Offset;

        m_Offset += actualSize;
        //if (m_Offset >= m_PageSize)
        //    __debugbreak();

        return alloc;
    }

    void LinearAllocator::CleanupUsedPages(uint64_t fenceValue)
    {
        if (m_CurrentPage == nullptr)
            return;

        m_RetiredPages.push_back(m_CurrentPage);
        m_CurrentPage = nullptr;
        m_Offset = 0;

        s_PageManagers[m_AllocationType].DiscardPages(fenceValue, m_RetiredPages);
        m_RetiredPages.clear();

        s_PageManagers[m_AllocationType].FreeLargePages(fenceValue, m_RetiredPages);
        m_LargePageList.clear();
    }

    LinearAllocator::LinearAllocatorPageManager::LinearAllocatorPageManager(AllocatorType type)
        : m_Type(type)
    {

    }

    LinearAllocator::LinearAllocatorPage* LinearAllocator::LinearAllocatorPageManager::RequestPage()
    {
        while (!m_RetiredPages.empty() && D3D12Renderer::CommandQueueManager.IsFenceComplete(m_RetiredPages.front().FenceValue))
        {
            m_AvailablePages.push(m_RetiredPages.front().Page);
            m_RetiredPages.pop();
        }

        LinearAllocatorPage* page = nullptr;

        if (!m_AvailablePages.empty()) {
            page = m_AvailablePages.front();
            m_AvailablePages.pop();
        }
        else {
            page = CreateNewPage();
            m_PagePool.emplace_back(page);
        }
        return page;
    }

    LinearAllocator::LinearAllocatorPage* LinearAllocator::LinearAllocatorPageManager::CreateNewPage(size_t size /*= 0*/)
    {
        D3D12_HEAP_PROPERTIES props;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask = 0;
        props.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC desc;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc = { 1, 0 };
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_RESOURCE_STATES defaultState;

        if (m_Type == AllocatorType::GpuExclusive) {
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            desc.Width = size == 0 ? AllocatorPageSize::GPU : size;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            defaultState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        else {
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            desc.Width = size == 0 ? AllocatorPageSize::CPU : size;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            defaultState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }

        ID3D12Resource* resource = nullptr;
        D3D12::ThrowIfFailed(D3D12Renderer::Context->DeviceResources->Device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            defaultState,
            nullptr,
            IID_PPV_ARGS(&resource)
        ));
        
        resource->SetName(L"LinearAllocator Page");
        return new LinearAllocator::LinearAllocatorPage(resource, defaultState);
    }

    void LinearAllocator::LinearAllocatorPageManager::DiscardPages(uint64_t fenceValue, const std::vector<LinearAllocatorPage*>& pages)
    {
        for (auto iter = pages.begin(); iter != pages.end(); iter++) {
            m_RetiredPages.push({ fenceValue, *iter });
        }
    }

    void LinearAllocator::LinearAllocatorPageManager::FreeLargePages(uint64_t fenceValue, const std::vector<LinearAllocatorPage*>& pages)
    {
        while (!m_DeletionQueue.empty() && D3D12Renderer::CommandQueueManager.IsFenceComplete(m_DeletionQueue.front().FenceValue))
        {
            delete m_DeletionQueue.front().Page;
            m_DeletionQueue.pop();
        }

        for (auto iter = pages.begin(); iter != pages.end(); iter++) {
            (*iter)->Unmap();
            m_DeletionQueue.push({ fenceValue, *iter });
        }
    }

    void LinearAllocator::LinearAllocatorPageManager::Destroy()
    {
        m_PagePool.clear();
    }

    DynamicAllocation LinearAllocator::AllocateLargePage(size_t sizeInBytes)
    {
        LinearAllocatorPage* page = s_PageManagers[m_AllocationType].CreateNewPage(sizeInBytes);
        m_LargePageList.push_back(page);

        DynamicAllocation alloc(*page, sizeInBytes, 0);
        alloc.CpuAddress = page->m_CpuVirtualAddress;
        alloc.GpuAddress = page->GetGPUAddress();
        return alloc;
    }

}
