#pragma once

#include <set>
#include "d3d12.h"

#include "Platform/D3D12/ComPtr.h"

namespace Hazel {

    class D3D12Texture2D;

    struct HeapAllocationDescription
    {
    public:
        size_t OffsetInHeap;
        size_t Range;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
    };

    class D3D12DescriptorHeap
    {
    public:
        D3D12DescriptorHeap(ID3D12DescriptorHeap* pExistingHeap) noexcept;
        D3D12DescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc) noexcept(false);
        D3D12DescriptorHeap(
            ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            D3D12_DESCRIPTOR_HEAP_FLAGS flags,
            size_t count) noexcept(false);
        D3D12DescriptorHeap(
            ID3D12Device* device,
            size_t count) noexcept(false) :
            D3D12DescriptorHeap(device,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, count) {}

        D3D12DescriptorHeap(D3D12DescriptorHeap&&) = default;
        D3D12DescriptorHeap& operator=(D3D12DescriptorHeap&&) = default;

        D3D12DescriptorHeap(const D3D12DescriptorHeap&) = delete;
        D3D12DescriptorHeap& operator=(const D3D12DescriptorHeap&) = delete;

        bool Allocate(Ref<D3D12Texture2D>& texture);
        bool Allocate(HeapAllocationDescription& allocation);

        size_t GetCount() const noexcept { return m_Description.NumDescriptors; }
        unsigned int GetFlags() const noexcept { return m_Description.Flags; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const noexcept { return m_Description.Type; }
        size_t GetIncrement() const noexcept { return m_IncrementSize; }
        ID3D12DescriptorHeap* GetHeap() const noexcept { return m_Heap.Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE GetFirstCPUHandle() const noexcept;
        D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGPUHandle() const noexcept;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(size_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(size_t index) const;

    private:
        void Create(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc);

        uint32_t GetAvailableDescriptor();
        void     ReleaseDescriptor(size_t index);

        TComPtr<ID3D12DescriptorHeap> m_Heap;

        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
        D3D12_DESCRIPTOR_HEAP_DESC  m_Description;
        uint32_t                    m_IncrementSize;
        std::set<size_t>            m_FreeDescriptors;
    };

}