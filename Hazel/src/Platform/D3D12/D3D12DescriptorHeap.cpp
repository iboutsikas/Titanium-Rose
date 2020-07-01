#include "hzpch.h"
#include "Platform/D3D12/D3D12DescriptorHeap.h"
#include "Platform/D3D12/D3D12Helpers.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel
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

        for (size_t i = 0; i < m_Description.NumDescriptors; i++)
        {
            m_FreeDescriptors.insert(i);
        }
    }

    D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc) noexcept(false)
    {
        Create(device, pDesc);
    }

    D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, size_t count) noexcept(false)
    {
        HZ_CORE_ASSERT(count < UINT32_MAX, "That's a lot of descriptors!!");

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Flags = flags;
        desc.NumDescriptors = count;
        desc.Type = type;
        Create(device, &desc);
    }


    bool D3D12DescriptorHeap::Allocate(Ref<D3D12Texture2D>& texture)
    {
        if (m_FreeDescriptors.empty()) {
            return false;
        }

        uint32_t offset = GetAvailableDescriptor();

        texture->m_DescriptorAllocation.OffsetInHeap = offset;
        texture->m_DescriptorAllocation.CPUHandle = GetCPUHandle(offset);
        texture->m_DescriptorAllocation.GPUHandle = GetGPUHandle(offset);

        return true;
    }

    bool D3D12DescriptorHeap::Allocate(HeapAllocationDescription& allocation)
    {
        if (m_FreeDescriptors.empty()) {
            return false;
        }

        uint32_t offset = GetAvailableDescriptor();

        allocation.OffsetInHeap = offset;
        allocation.CPUHandle = GetCPUHandle(offset);
        allocation.GPUHandle = GetGPUHandle(offset);

        return true;
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
            !(m_Description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
            "Heap is not shader visible"
        );
        HZ_CORE_ASSERT(index < m_Description.NumDescriptors, "Index is out of the available descriptors");

        D3D12_GPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = m_GPUHandle.ptr + static_cast<uint64_t>(index) * static_cast<uint64_t>(m_IncrementSize);;
        return handle;
    }

    void D3D12DescriptorHeap::Create(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc)
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
            for (size_t i = 0; i < m_Description.NumDescriptors; i++)
            {
                m_FreeDescriptors.insert(i);
            }
        }
    }
    uint32_t D3D12DescriptorHeap::GetAvailableDescriptor()
    {
        HZ_CORE_ASSERT(m_FreeDescriptors.size() != 0, "We run out of descriptors in this heap");
        auto iter = this->m_FreeDescriptors.begin();
        auto tileNumber = *iter;
        this->m_FreeDescriptors.erase(iter);
        return tileNumber;
    }
    void D3D12DescriptorHeap::ReleaseDescriptor(size_t index)
    {
        HZ_CORE_ASSERT(index < m_Description.NumDescriptors, "Index is out of bounds");
        m_FreeDescriptors.insert(index);
    }
}


