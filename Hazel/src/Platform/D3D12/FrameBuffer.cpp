#include "hzpch.h"
#include "Platform/D3D12/Framebuffer.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel {
    Ref<FrameBuffer> FrameBuffer::Create(D3D12ResourceBatch& batch, FrameBufferOptions& opts)
    {
        Ref<D3D12Texture2D> colorTexture = nullptr;
        Ref<D3D12Texture2D> depthStencilTexture = nullptr;
        Ref<FrameBuffer> ret = Ref<FrameBuffer>(new FrameBuffer());

        if (opts.ColourFormat != DXGI_FORMAT_UNKNOWN)
        {
            D3D12Texture::TextureCreationOptions textureOpts;
            textureOpts.Name = opts.Name + "color";
            textureOpts.Format = opts.ColourFormat;
            textureOpts.Width = opts.Width;
            textureOpts.Height = opts.Height;
            textureOpts.Depth = 1;
            textureOpts.MipLevels = 1;
            textureOpts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            colorTexture = D3D12Texture2D::CreateCommittedTexture(batch, textureOpts);
        }

        if (opts.DepthStencilFormat != DXGI_FORMAT_UNKNOWN)
        {
            D3D12Texture::TextureCreationOptions textureOpts;
            textureOpts.Name = opts.Name + "depthstencil";
            textureOpts.Format = opts.DepthStencilFormat;
            textureOpts.Width = opts.Width;
            textureOpts.Height = opts.Height;
            textureOpts.Depth = 1;
            textureOpts.MipLevels = 1;
            textureOpts.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            textureOpts.IsDepthStencil = true;

            depthStencilTexture = D3D12Texture2D::CreateCommittedTexture(batch, textureOpts);
        }

        ret->ColorResource.swap(colorTexture);
        ret->DepthStensilResource.swap(depthStencilTexture);
        
        return ret;
    }
    FrameBuffer::FrameBuffer()
    {
    }
}

