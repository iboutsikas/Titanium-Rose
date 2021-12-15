#include "trpch.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Roses
{
    D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12DescriptorHeap* pExistingHeap) noexcept
        : m_Heap(pExistingHeap)
    {
        m_CPUHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();
        m_GPUHandle = m_Heap->GetGPUDescriptorHandleForHeapStart();
        m_Description = m_Heap->GetDesc();

        TComPtr<ID3D12Device> device;
        pExistingHeap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

        m_IncrementSize = device->GetDescriptorHandleIncrementSize(m_Description.Type);

        m_FreeDescriptors.emplace_front(0, m_Description.NumDescriptors);
    }

    D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc) noexcept(false)
    {
        Initialize(device, pDesc);
    }

    D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, size_t count) noexcept(false)
    {
        HZ_CORE_ASSERT(count < UINT32_MAX, "That's a lot of descriptors!!");

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Flags = flags;
        desc.NumDescriptors = count;
        desc.Type = type;
        Initialize(device, &desc);
    }

    HeapAllocationDescription D3D12DescriptorHeap::Allocate(size_t numDescriptors)
    {
        HeapAllocationDescription allocation;

        if (m_FreeDescriptors.empty()) {
            return allocation;
        }

        size_t offset = GetAvailableDescirptorRange(numDescriptors);

        if (offset != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            allocation.Allocated = true;
            allocation.OffsetInHeap = offset;
            allocation.Range = numDescriptors;
            allocation.CPUHandle = GetCPUHandle(offset);
            if ((m_Description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
            {
                allocation.GPUHandle = GetGPUHandle(offset);
            }
        }

        return allocation;
    }

    void D3D12DescriptorHeap::Release(HeapAllocationDescription& allocation)
    {
        if (!allocation.Allocated)
            return;

        // NOTE: We could combine this loop to the one below
        for (auto& range : m_FreeDescriptors)
        {
            // We can add the allocation to the begining of this range
            if (allocation.OffsetInHeap + allocation.Range == range.Start)
            {
                range.Start = allocation.OffsetInHeap;
                range.Count += allocation.Range;
                goto ret;
            }
            // We can add the allocation to the end of this range
            else if ((range.Start + range.Count) == allocation.OffsetInHeap)
            {
                range.Count += allocation.Range;
                goto ret;
            }
        }
        
        // At this point the for loop has finished so we need to add a new range
        for (auto it = m_FreeDescriptors.begin(); it != m_FreeDescriptors.end(); ++it)
        {
            auto next = std::next(it);

            // That means we only have a single free section. We need to either add
            // the whole allocation before or after the current one
            if (next == m_FreeDescriptors.end())
            {
                if (allocation.OffsetInHeap + allocation.Range < it->Start)
                {
                    m_FreeDescriptors.emplace_front(allocation.OffsetInHeap, allocation.Range);
                }
                else if (allocation.OffsetInHeap > it->Start + it->Count)
                {
                    m_FreeDescriptors.emplace_back(allocation.OffsetInHeap, allocation.Range);
                }
                else
                {
                    goto invalid_path;
                }
                goto ret;
            }
            
            if((allocation.OffsetInHeap > it->Start + it->Count)            // The allocation release must start AFTER it
            && (allocation.OffsetInHeap + allocation.Range < next->Start)   // The allocation must be BEFORE next
            && (next->Start - (it->Start + it->Count) >= allocation.Range))
            {
                m_FreeDescriptors.emplace(next, allocation.OffsetInHeap, allocation.Range);
                goto ret;
            }
        }
invalid_path:
        HZ_CORE_ASSERT(false, "This execution path should not be reachable. There is probably a dynamic resource that is assumed discarded, but it is not");
ret:
        allocation.Allocated = false;
        allocation.CPUHandle.ptr = allocation.GPUHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    size_t D3D12DescriptorHeap::GetFreeDescriptorCount() const noexcept
    {
        size_t accumulator = 0;
        
        for (auto& range : m_FreeDescriptors)
        {
            accumulator += range.Count;
        }

        return accumulator;

    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetFirstCPUHandle() const noexcept
    {
        HZ_CORE_ASSERT(m_Heap != nullptr, "Heap is uninitialized");

        return m_CPUHandle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetFirstGPUHandle() const noexcept
    {
        HZ_CORE_ASSERT(m_Heap != nullptr, "Heap is uninitialized");
        HZ_CORE_ASSERT(
            !(m_Description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE), 
            "Heap is not shader visible"
        );
        return m_GPUHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUHandle(size_t index) const
    {
        HZ_CORE_ASSERT(m_Heap != nullptr, "Heap is uninitialized");
        HZ_CORE_ASSERT(index < m_Description.NumDescriptors, "Index is out of the available descriptors");

        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = m_CPUHandle.ptr + static_cast<uint64_t>(index) * static_cast<uint64_t>(m_IncrementSize);;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHandle(size_t index) const
    {
        HZ_CORE_ASSERT(m_Heap != nullptr, "Heap is uninitialized");
        HZ_CORE_ASSERT(
            (m_Description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
            "Heap is not shader visible"
        );
        HZ_CORE_ASSERT(index < m_Description.NumDescriptors, "Index is out of the available descriptors");

        D3D12_GPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = m_GPUHandle.ptr + static_cast<uint64_t>(index) * static_cast<uint64_t>(m_IncrementSize);;
        return handle;
    }

    void D3D12DescriptorHeap::Initialize(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc)
    {
        HZ_CORE_ASSERT(pDesc != nullptr, "Heap Description cannot be null");

        m_Description = *pDesc;
        m_IncrementSize = pDevice->GetDescriptorHandleIncrementSize(pDesc->Type);

        if (m_Description.NumDescriptors == 0)
        {
            m_Heap.Reset();
            m_CPUHandle.ptr = 0;
            m_GPUHandle.ptr = 0;
            m_FreeDescriptors.clear();
        }
        else
        {
            D3D12::ThrowIfFailed(pDevice->CreateDescriptorHeap(
                &m_Description,
                IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())
            ));

            m_CPUHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();

            if (m_Description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
            {
                m_GPUHandle = m_Heap->GetGPUDescriptorHandleForHeapStart();
            }

            m_FreeDescriptors.emplace_front(0, m_Description.NumDescriptors);
        }
    }

    size_t D3D12DescriptorHeap::GetAvailableDescirptorRange(size_t numDescriptors)
    {
        if (numDescriptors > m_Description.NumDescriptors)
        {
            HZ_CORE_ERROR("Tried to allocate {0} descriptors, but the heap has {1} total", numDescriptors, m_Description.NumDescriptors);
            return D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        auto r = m_FreeDescriptors.end();
        size_t offset = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        for (auto range = m_FreeDescriptors.begin(); range != m_FreeDescriptors.end(); ++range) {
            if (range->Count < numDescriptors) {
                continue;
            }
        
            offset = range->Start;
            range->Start += (numDescriptors);
            range->Count -= (numDescriptors);
            r = range;
            break;
        }

        // We couldn't find a range to allocate
        if (r == m_FreeDescriptors.end())
        {
            return D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        }

        if (r->Count == 0)
        {
            m_FreeDescriptors.erase(r);
        }

        return offset;
    }
}


