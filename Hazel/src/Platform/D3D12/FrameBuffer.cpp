#include "hzpch.h"
#include "Platform/D3D12/Framebuffer.h"
#include "Platform/D3D12/D3D12Texture.h"

namespace Hazel {
    Ref<FrameBuffer> FrameBuffer::Create(FrameBufferOptions& opts)
    {
        Ref<Texture2D> colorTexture = nullptr;
        Ref<Texture2D> depthStencilTexture = nullptr;
        Ref<FrameBuffer> ret = Ref<FrameBuffer>(new FrameBuffer());

        if (opts.ColourFormat != DXGI_FORMAT_UNKNOWN)
        {
            TextureCreationOptions textureOpts;
            textureOpts.Name = opts.Name + ":color";
            textureOpts.Format = opts.ColourFormat;
            textureOpts.Width = opts.Width;
            textureOpts.Height = opts.Height;
            textureOpts.Depth = 1;
            textureOpts.MipLevels = 1;
            textureOpts.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            colorTexture = Texture2D::CreateCommittedTexture(textureOpts);
        }

        if (opts.DepthStencilFormat != DXGI_FORMAT_UNKNOWN)
        {
            TextureCreationOptions textureOpts;
            textureOpts.Name = opts.Name + ":depth-stencil";
            textureOpts.Format = opts.DepthStencilFormat;
            textureOpts.Width = opts.Width;
            textureOpts.Height = opts.Height;
            textureOpts.Depth = 1;
            textureOpts.MipLevels = 1;
            textureOpts.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            textureOpts.IsDepthStencil = true;

            depthStencilTexture = Texture2D::CreateCommittedTexture(textureOpts);
        }

        ret->ColorResource.swap(colorTexture);
        ret->DepthStensilResource.swap(depthStencilTexture);
        
        return ret;
    }
    FrameBuffer::FrameBuffer()
    {
    }
}

