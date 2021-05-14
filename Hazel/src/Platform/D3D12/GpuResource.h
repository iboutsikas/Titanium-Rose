#pragma once
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/HeapAllocationDescription.h"

namespace Hazel
{

    class GpuResource
    {
        friend class CommandContext;
        friend class GraphicsContext;
        friend class ComputeContext;

    public:
        GpuResource();
        GpuResource(std::string id, D3D12_RESOURCE_STATES state);

        inline ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        inline DXGI_FORMAT GetFormat() const { return m_Resource->GetDesc().Format; }
        inline std::string& GetIdentifier() { return m_Identifier; }

        void Reset();
       
        void SetName(std::string name);

        // Sets the internally tracked resource state. Should be used when the state has been 
        // set by some other API outside the direct engine code (i.e. DDSTextureLoader
        inline void BypassAndSetState(D3D12_RESOURCE_STATES newState) { m_CurrentState = newState; }

        HeapAllocationDescription SRVAllocation;
        HeapAllocationDescription UAVAllocation;
        HeapAllocationDescription RTVAllocation;

    protected:
        D3D12_RESOURCE_STATES m_CurrentState;
        D3D12_RESOURCE_STATES m_TransitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
        TComPtr<ID3D12Resource> m_Resource;
        std::string m_Identifier;

    };
}


