#pragma once


#include "Hazel/Renderer/Texture.h"

#include "d3d12.h"
#include "Platform/D3D12/ComPtr.h"
#include "Platform/D3D12/d3dx12.h"


namespace Hazel {
	class D3D12Context;

	class D3D12Texture2D : public Texture2D
	{
	public:
		D3D12Texture2D(uint32_t width, uint32_t height);
		D3D12Texture2D(const std::string& path);

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;
		
		void Transition(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
		void Transition(D3D12_RESOURCE_STATES to);

		inline ID3D12Resource* GetCommitedResource() const { return m_CommittedResource.Get(); }
		inline ID3D12Resource* GetUploadResource() const { return m_UploadResource.Get(); }
		/*inline CD3DX12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() { return m_GPUHandle; }
		inline CD3DX12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() { return m_CPUHandle; }*/
		void DebugNameResource(std::wstring& name);
	private:
		std::string m_Path;
		uint32_t m_Width;
		uint32_t m_Height;
		D3D12_RESOURCE_STATES m_CurrentState;
		D3D12Context* m_Context;
		TComPtr<ID3D12Resource> m_CommittedResource;
		TComPtr<ID3D12Resource> m_UploadResource;
		/*CD3DX12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;*/
	};
}