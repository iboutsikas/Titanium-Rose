#include "hzpch.h"
#include "Hazel/Renderer/Buffer.h"

#include "Hazel/Renderer/Renderer.h"

#include "Platform/D3D12/D3D12Buffer.h"
namespace Hazel {

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
			case RendererAPI::API::D3D12:	return CreateRef<D3D12VertexBuffer>(vertices, size);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  HZ_CORE_ASSERT(false, "RendererAPI::OpenGL is currently not supported!"); return nullptr;
			case RendererAPI::API::D3D12:	return CreateRef<D3D12IndexBuffer>(indices, count);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}