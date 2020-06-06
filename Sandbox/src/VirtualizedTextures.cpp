#include "VirtualizedTextures.h"

#include "Platform/D3D12/D3D12Texture.h"
#include "Platform/D3D12/D3D12UploadBatch.h"

#include "ModelLoader.h"

VirtualizedTextures::VirtualizedTextures()
    : Layer("Virtual Textures"),
    m_CameraController(glm::vec3(0.0f, 0.0f, 30.0f), 28.0f, (1280.0f / 720.0f), 0.1f, 100.0f),
    m_Context(nullptr)
{
    m_Context = static_cast<Hazel::D3D12Context*>(Hazel::Application::Get().GetWindow().GetContext());
    m_Textures.resize(Textures::TextureCount);
    m_Models.resize(Models::ModelCount);

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    m_Context->DeviceResources->Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
    if (options.TiledResourcesTier == D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED)
    {
        HZ_ERROR("Tiling is not supported");
    }


    Hazel::D3D12ResourceUploadBatch batch(m_Context->DeviceResources->Device);
    auto cmdlist = batch.Begin();

    m_Textures[Textures::EarthTexture] = Hazel::CreateRef<Hazel::D3D12Texture2D>("assets/textures/earth.dds", cmdlist.Get());
    m_Textures[Textures::EarthTexture]->Transition(cmdlist.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_Textures[Textures::CubeTexture] = Hazel::CreateRef<Hazel::D3D12Texture2D>("assets/textures/test_texture.dds", cmdlist.Get());
    m_Textures[Textures::CubeTexture]->Transition(cmdlist.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_Models[Models::SphereModel] = ModelLoader::LoadFromFile(std::string("assets/models/test_sphere.glb"));

    batch.End(m_Context->DeviceResources->CommandQueue.Get()).wait();

    D3D12_RESOURCE_DESC reservedTextureDesc = {};
    reservedTextureDesc.MipLevels = 13;
    reservedTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    reservedTextureDesc.Width = 4096;
    reservedTextureDesc.Height = 2048;
    reservedTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    reservedTextureDesc.DepthOrArraySize = 1;
    reservedTextureDesc.SampleDesc.Count = 1;
    reservedTextureDesc.SampleDesc.Quality = 0;
    reservedTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    reservedTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

    auto dev = m_Context->DeviceResources->Device;
    // each pixel is 4 bytes
    D3D12_HEAP_DESC heapDesc = CD3DX12_HEAP_DESC(4096 * 2048 * 4, D3D12_HEAP_TYPE_DEFAULT);
    Hazel::D3D12::ThrowIfFailed(dev->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_Heap)));

    Hazel::D3D12::ThrowIfFailed(dev->CreateReservedResource(
        &reservedTextureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&m_VirtualTexture)
    ));

    UINT numTiles = 0;
    D3D12_TILE_SHAPE tileShape = {};
    UINT subresourceCount = reservedTextureDesc.MipLevels;
    D3D12_PACKED_MIP_INFO mipInfo;
    std::vector<D3D12_SUBRESOURCE_TILING> tilings(subresourceCount);

    dev->GetResourceTiling(m_VirtualTexture.Get(), &numTiles, &mipInfo,
        &tileShape, &subresourceCount, 0, &tilings[0]);

    D3D12_TILED_RESOURCE_COORDINATE regionStartCoordinates[] = {
        {0, 0, 0, 0},
        {1, 0, 0, 0}
    };

    D3D12_TILE_REGION_SIZE regionSizes[] = {
        {1, TRUE, 1, 1, 1},
        {1, TRUE, 1, 1, 1}
    };

    D3D12_TILE_RANGE_FLAGS rangeFlags[] = {
        D3D12_TILE_RANGE_FLAG_NONE,
        D3D12_TILE_RANGE_FLAG_NONE
    };

    UINT rangeOffsets[] = { 0, 1 };

    UINT rangeTileCounts[] = { 1, 1 };

    m_Context->DeviceResources->CommandQueue->UpdateTileMappings(
        m_VirtualTexture.Get(),
        _countof(regionStartCoordinates),
        regionStartCoordinates,
        regionSizes,
        m_Heap.Get(),
        2,
        rangeFlags,
        rangeOffsets,
        rangeTileCounts,
        D3D12_TILE_MAPPING_FLAG_NONE
    );

    {
        D3D12_RESOURCE_DESC reservedTextureDesc = {};
        reservedTextureDesc.MipLevels = 1;
        reservedTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        reservedTextureDesc.Width = 4096;
        reservedTextureDesc.Height = 2048;
        reservedTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        reservedTextureDesc.DepthOrArraySize = 1;
        reservedTextureDesc.SampleDesc.Count = 1;
        reservedTextureDesc.SampleDesc.Quality = 0;
        reservedTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        reservedTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

        Hazel::D3D12::ThrowIfFailed(dev->CreateReservedResource(
            &reservedTextureDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_PPV_ARGS(&m_HeapTexture)
        ));

        D3D12_TILE_RANGE_FLAGS RangeFlags = D3D12_TILE_RANGE_FLAG_NONE;
        m_Context->DeviceResources->CommandQueue->UpdateTileMappings(
            m_HeapTexture.Get(),
            1, nullptr, nullptr,
            m_Heap.Get(),
            1, &RangeFlags, nullptr, nullptr,
            D3D12_TILE_MAPPING_FLAG_NONE
        );
    }

    m_ResourceHeap = m_Context->DeviceResources->CreateDescriptorHeap(
        dev,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        15,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    );


    m_RTVHeap = m_Context->DeviceResources->CreateDescriptorHeap(
        dev,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        1
    );
}

void VirtualizedTextures::OnAttach()
{
    m_MainObject = Hazel::CreateRef<Hazel::GameObject>();
    m_MainObject->Transform.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    m_MainObject->Transform.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
    m_MainObject->Name = "Main Object";
    m_MainObject->Mesh = m_Models[Models::SphereModel]->Mesh;
    m_MainObject->Material = Hazel::CreateRef<Hazel::HMaterial>();
    m_MainObject->Material->Glossines = 32.0f;
    m_MainObject->Material->Color = glm::vec4(1.0f);
}

void VirtualizedTextures::OnDetach()
{
}

void VirtualizedTextures::OnUpdate(Hazel::Timestep ts)
{
    m_CameraController.OnUpdate(ts);
    auto cmdList = m_Context->DeviceResources->CommandList;
}

void VirtualizedTextures::OnImGuiRender()
{
}

void VirtualizedTextures::OnEvent(Hazel::Event& e)
{
}
