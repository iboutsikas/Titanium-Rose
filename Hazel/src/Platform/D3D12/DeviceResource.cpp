#include "hzpch.h"

#include "Platform/D3D12/d3dx12.h"
#include "Platform/D3D12/DeviceResource.h"

namespace Hazel
{

    DeviceResource::DeviceResource() : 
        m_Width(0), 
        m_Height(0), 
        m_Depth(0), 
        m_Identifier(""),
        m_CurrentState(D3D12_RESOURCE_STATE_COMMON),
        m_Resource(nullptr)
    {

    }

    DeviceResource::DeviceResource(uint32_t width, uint32_t height, uint32_t depth, std::string id, D3D12_RESOURCE_STATES state) :
        m_Width(width),
        m_Height(height),
        m_Depth(depth),
        m_Identifier(id),
        m_CurrentState(state),
        m_Resource(nullptr)
    {

    }

    void DeviceResource::Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Tried to create transition for null resource");
        batch.GetCommandList()->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_Resource.Get(),
                from,
                to
            )
        );
        m_CurrentState = to;
    }

    void DeviceResource::Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES to)
    {
        if (to == m_CurrentState)
            return;
        this->Transition(batch, m_CurrentState, to);
    }

    void DeviceResource::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
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

    void DeviceResource::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES to)
    {
        if (to == m_CurrentState)
            return;
        this->Transition(commandList, m_CurrentState, to);
    }

    D3D12_RESOURCE_BARRIER DeviceResource::CreateTransition(D3D12_RESOURCE_STATES to)
    {
        HZ_CORE_ASSERT(m_Resource != nullptr, "Tried to create transition for null resource");
        return CD3DX12_RESOURCE_BARRIER::Transition(m_Resource.Get(), m_CurrentState, to);
    }

    void DeviceResource::SetName(std::string name)
    {
        m_Identifier = name;

        std::wstring wPath(m_Identifier.begin(), m_Identifier.end());

        m_Resource->SetName(wPath.c_str());
    }

}
