#pragma once

#include "Hazel.h"
#include "Hazel/Renderer/PerspectiveCameraController.h"

#include "Platform/D3D12/D3D12Context.h"

#include "vector"

class VirtualizedTextures : public Hazel::Layer
{
public:

	VirtualizedTextures();
	virtual ~VirtualizedTextures() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;

private:
	Hazel::PerspectiveCameraController m_CameraController;
	Hazel::D3D12Context* m_Context;
	// We try to render to this one, and it maps to the heap
	Hazel::TComPtr<ID3D12Resource> m_VirtualTexture;
	Hazel::TComPtr<ID3D12Heap> m_Heap;
	// This should be used to view the whole heap
	// as a texture
	Hazel::TComPtr<ID3D12Resource> m_HeapTexture;
	Hazel::TComPtr<ID3D12DescriptorHeap> m_ResourceHeap;
	Hazel::TComPtr<ID3D12DescriptorHeap> m_RTVHeap;

	Hazel::Ref<Hazel::GameObject> m_MainObject;

	enum Textures {
		EarthTexture,
		CubeTexture,
		TextureCount
	};
	std::vector<Hazel::Ref<Hazel::D3D12Texture2D>> m_Textures;

	enum Models {
		SphereModel,
		ModelCount
	};
	std::vector<Hazel::Ref<Hazel::GameObject>> m_Models;
};

