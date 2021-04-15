#pragma once
#include "Hazel/Core/Core.h"

#include "Platform/D3D12/ComPtr.h"

#include <d3d12.h>

namespace Hazel
{
    class D3D12ResourceBatch; 

    class DeviceResource
    {
    public:
        DeviceResource();
        DeviceResource(uint32_t width, uint32_t height, uint32_t depth, std::string id, D3D12_RESOURCE_STATES state);

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint32_t GetDepth() const { return m_Depth; }

        inline ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        inline DXGI_FORMAT GetFormat() const { return m_Resource->GetDesc().Format; }
        inline std::string GetIdentifier() const { return m_Identifier; }
        // Sets the current state WITHOUT performing a transition
        // Only for use when the transition has been batched.
        inline void SetCurrentState(D3D12_RESOURCE_STATES state) { m_CurrentState = state; }

        void Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
        void Transition(D3D12ResourceBatch& batch, D3D12_RESOURCE_STATES to);
        void Transition(TComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
        void Transition(TComPtr<ID3D12GraphicsCommandList> commandList, D3D12_RESOURCE_STATES to);
        D3D12_RESOURCE_BARRIER CreateTransition(D3D12_RESOURCE_STATES to);

        void SetName(std::string name);


    protected:
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Depth;
        std::string m_Identifier;
        D3D12_RESOURCE_STATES m_CurrentState;
        TComPtr<ID3D12Resource> m_Resource;

    protected:

    };
}


