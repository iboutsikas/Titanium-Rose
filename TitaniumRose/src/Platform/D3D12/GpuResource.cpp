#include "trpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/GpuResource.h"
#include "Platform/D3D12/D3D12ResourceBatch.h"

namespace Roses
{

    GpuResource::GpuResource() : 
        m_CurrentState(D3D12_RESOURCE_STATE_COMMON),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1),
        m_GpuVirtualAddress((D3D12_GPU_VIRTUAL_ADDRESS)0),
        m_Identifier(""),
        m_Resource(nullptr)
    {

    }

    GpuResource::GpuResource(std::string id, D3D12_RESOURCE_STATES state) :
        m_CurrentState(state),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1),
        m_GpuVirtualAddress((D3D12_GPU_VIRTUAL_ADDRESS)0),
        m_Identifier(id),
        m_Resource(nullptr)
    {

    }
#if 0
    void GpuResource::Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Tried to create transition for null resource");
        batch.GetCommandList()->Get()->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_Resource.Get(),
                from,
                to
            )
        );
        m_CurrentState = to;
    }

    void GpuResource::Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES to)
    {
        if (to == m_CurrentState)
            return;
        this->Transition(batch, m_CurrentState, to);
    }

    void GpuResource::Transition(TComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Tried to create transition for null resource");
        commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_Resource.Get(),
                from,
                to
            )
        );
        m_CurrentState = to;
    }

    void GpuResource::Transition(TComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES to)
    {
        if (to == m_CurrentState)
            return;
        this->Transition(commandList, m_CurrentState, to);
    }

    D3D12_RESOURCE_BARRIER GpuResource::CreateTransition(D3D12_RESOURCE_STATES to)
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Tried to create transition for null resource");
        return CD3DX12_RESOURCE_BARRIER::Transition(m_Resource.Get(), m_CurrentState, to);
    }
#endif
    void GpuResource::Reset()
    {
        m_Resource = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    void GpuResource::Release()
    {
        D3D12::ThrowIfFailed(m_Resource->Release());
    }


    void GpuResource::SetName(std::string name)
    {
        m_Identifier = name;

        std::wstring wPath(m_Identifier.begin(), m_Identifier.end());

        m_Resource->SetName(wPath.c_str());
    }

}
