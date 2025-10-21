#include "pch.h"

#include <filesystem>

#include "CBufferTypes.h"
#include "CameraComponent.h"
#include "Renderer.h"
#include "TileLightManager.h"

void FTileLightManager::Initialize(URenderer* InRenderer)
{
    assert(InRenderer && "Initialization not done: Renderer is null");

    /** @note Initialization order is important due to dependency upon Renderer of other methods */
    Renderer = InRenderer;

    CreateShader("LightCullingShader.hlsl", "mainCS");

    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    Renderer->GetRHIDevice()->GetSwapChain()->GetDesc(&SwapChainDesc);

    OnResize(SwapChainDesc.BufferDesc.Width, SwapChainDesc.BufferDesc.Height);
}

void FTileLightManager::OnResize(uint32 InWidth, uint32 InHeight)
{
    if (Width == InWidth && Height == InHeight) { return; }

    Width = InWidth;
    
    Height = InHeight;

    CreateBuffer();
    
    CreateHeatmapTexture();
}

void FTileLightManager::CullPointLights(UCameraComponent* InCameraComponent, FViewport* InViewport, FLightingBufferType& LightingBuffer)
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11DeviceContext* DeviceContext = Renderer->GetRHIDevice()->GetDeviceContext();

    // --- Update Constant Buffers ---
    UpdateConstantBuffer();
    UpdateConstantBuffer(InCameraComponent, InViewport->GetSizeX() / static_cast<float>(InViewport->GetSizeY()));
    UpdateConstantBuffer(InViewport);
    UpdateConstantBuffer(LightingBuffer);

    // --- Set Compute Shader ---
    DeviceContext->CSSetShader(ComputeShader.Get(), nullptr, 0);

    // --- Set Constant Buffers ---
    ID3D11Buffer* ConstantBuffers[] = { CameraInfoConstantBuffer.Get(), ViewportConstantBuffer.Get(), TileConstantBuffer.Get(), LightingConstantBuffer.Get() };
    DeviceContext->CSSetConstantBuffers(0, 4, ConstantBuffers);

    // --- Set Shader Resource Views ---
    ID3D11ShaderResourceView* ShaderResourceViews[] = { static_cast<D3D11RHI*>(Renderer->GetRHIDevice())->GetDepthSRV() };
    DeviceContext->CSSetShaderResources(0, 1, ShaderResourceViews);

    // --- Set Unordered Access Views ---
    ID3D11UnorderedAccessView* UnorderedAccessViews[] = { PointLightMaskBufferUAV.Get(), nullptr, HeatmapTextureUAV.Get() };
    DeviceContext->CSSetUnorderedAccessViews(0, 3, UnorderedAccessViews, nullptr);

    // --- Dispatch ---
    uint32 DispatchWidth = (Width + TILE_WIDTH - 1) / TILE_WIDTH;
    uint32 DispatchHeight = (Height + TILE_HEIGHT - 1) / TILE_HEIGHT;
    DeviceContext->Dispatch(DispatchWidth, DispatchHeight, 1);

    // --- Unbind Resources ---
    ID3D11Buffer* NullConstantBuffers[] = { nullptr, nullptr, nullptr, nullptr };
    DeviceContext->CSSetConstantBuffers(0, 4, NullConstantBuffers);

    ID3D11ShaderResourceView* NullShaderResourceViews[] = { nullptr };
    DeviceContext->CSSetShaderResources(0, 1, NullShaderResourceViews);

    ID3D11UnorderedAccessView* NullUnorderedAccessViews[] = { nullptr, nullptr, nullptr };
    DeviceContext->CSSetUnorderedAccessViews(0, 3, NullUnorderedAccessViews, nullptr);
}

void FTileLightManager::RenderPointLightHeatmap()
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11DeviceContext* DeviceContext = Renderer->GetRHIDevice()->GetDeviceContext();

    ID3D11ShaderResourceView* ShaderResourceViews[] = { HeatmapTextureSRV.Get() };
    DeviceContext->PSSetShaderResources(2, 1, ShaderResourceViews);
    
    Renderer->RenderPostProcessing(UResourceManager::GetInstance().Load<UShader>("LightCullingDebugShader.hlsl"));    

    ID3D11ShaderResourceView* NullShaderResourceViews[] = { nullptr };
    DeviceContext->PSSetShaderResources(2, 1, NullShaderResourceViews);
}

void FTileLightManager::CreateShader(const FString& InFilePath, const FString& InEntryPoint)
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11Device* Device = Renderer->GetRHIDevice()->GetDevice();

    std::filesystem::path FilePath(InFilePath);
    assert(std::filesystem::exists(FilePath) && "Shader file does not exist");

    Microsoft::WRL::ComPtr<ID3DBlob> ShaderBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob = nullptr;
    HRESULT hResult = D3DCompileFromFile(
        FilePath.wstring().c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        InEntryPoint.c_str(),
        "cs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG,
        0,
        &ShaderBlob,
        &ErrorBlob
    );

    if (FAILED(hResult))
    {
        if (ErrorBlob)
        {
            OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
            // UE_LOG("셰이더 컴파일 실패: %s", (char*)ErrorBlob->GetBufferPointer());
        }
        assert(false && "Failed to compile shader");
    }

    hResult = Device->CreateComputeShader(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), nullptr, ComputeShader.ReleaseAndGetAddressOf());
    assert(SUCCEEDED(hResult) && "Failed to create compute shader");
}

void FTileLightManager::CreateBuffer()
{
    assert(Renderer && "Initialization not done: Renderer is null");

    CreatePointLightBuffer();
    CreatePointLightMaskBuffer();
    CreateConstantBuffer();
}

void FTileLightManager::CreatePointLightBuffer()
{
    assert(Renderer && "Initialization not done: Renderer is null");
    
    // ID3D11Device* Device = Renderer->GetRHIDevice()->GetDevice();
    //
    // // --- Create PointLightBuffer ---
    //
    // D3D11_BUFFER_DESC BufferDesc    = {};
    // BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
    // BufferDesc.ByteWidth            = sizeof(FPointLightShaderParameters) * MAX_NUM_LIGHT_PER_TILE;
    // BufferDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
    // BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
    // BufferDesc.MiscFlags            = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    // BufferDesc.StructureByteStride  = sizeof(FPointLightShaderParameters);
    //
    // HRESULT hResult = Device->CreateBuffer(&BufferDesc, nullptr, &PointLightBuffer);
    // assert(SUCCEEDED(hResult) && "Failed to create point light buffer");
    //
    // // --- Create Shader Resource View for PointLightBuffer ---
    //
    // D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    // SRVDesc.Format                          = DXGI_FORMAT_UNKNOWN;
    // SRVDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
    // SRVDesc.Buffer.FirstElement             = 0;
    // SRVDesc.Buffer.NumElements              = MAX_NUM_LIGHT_PER_TILE;
    //
    // hResult = Device->CreateShaderResourceView(PointLightBuffer.Get(), &SRVDesc, &PointLightBufferSRV);
    // assert(SUCCEEDED(hResult) && "Failed to create point light buffer SRV");
}

void FTileLightManager::CreatePointLightMaskBuffer()
{
    assert(Renderer && "Initialization not done: Renderer is null");
    
    ID3D11Device* Device = Renderer->GetRHIDevice()->GetDevice();

    uint32 NumGroupsX = (Width + TILE_WIDTH - 1) / TILE_WIDTH;
    uint32 NumGroupsY = (Height + TILE_HEIGHT - 1) / TILE_HEIGHT;
    
    // --- Create PointLightMaskBuffer ---
    
    D3D11_BUFFER_DESC BufferDesc    = {};
    BufferDesc.Usage                = D3D11_USAGE_DEFAULT;
    BufferDesc.ByteWidth            = sizeof(uint32) * NUM_LIGHT_BUCKET_PER_TILE * NumGroupsX * NumGroupsY;
    BufferDesc.BindFlags            = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags       = 0;
    BufferDesc.MiscFlags            = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride  = sizeof(uint32);
    
    HRESULT hResult = Device->CreateBuffer(&BufferDesc, nullptr, &PointLightMaskBuffer);
    assert(SUCCEEDED(hResult) && "Failed to create point light mask buffer");
    
    // --- Create Unordererd Access View for PointLightMaskBuffer ---
    
    D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc    = {};
    UAVDesc.Format                              = DXGI_FORMAT_UNKNOWN;
    UAVDesc.ViewDimension                       = D3D11_UAV_DIMENSION_BUFFER;
    UAVDesc.Buffer.FirstElement                 = 0;
    UAVDesc.Buffer.NumElements                  = NUM_LIGHT_BUCKET_PER_TILE * NumGroupsX * NumGroupsY;
    
    hResult = Device->CreateUnorderedAccessView(PointLightMaskBuffer.Get(), &UAVDesc, &PointLightMaskBufferUAV);
    assert(SUCCEEDED(hResult) && "Failed to create point light mask buffer UAV");
    
    // --- Create Shader Resource View for PointLightBufferMask ---
    
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format                          = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.FirstElement             = 0;
    SRVDesc.Buffer.NumElements              = NUM_LIGHT_BUCKET_PER_TILE * NumGroupsX * NumGroupsY;
    
    hResult = Device->CreateShaderResourceView(PointLightMaskBuffer.Get(), &SRVDesc, &PointLightMaskBufferSRV);
    assert(SUCCEEDED(hResult) && "Failed to create point light mask buffer SRV");
}

void FTileLightManager::CreateHeatmapTexture()
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11Device* Device = Renderer->GetRHIDevice()->GetDevice();
    HRESULT hResult;

    // --- Create HeatmapBuffer ---
    {
        D3D11_TEXTURE2D_DESC TextureDesc    = {};
        TextureDesc.Width                   = Width;
        TextureDesc.Height                  = Height;
        TextureDesc.MipLevels               = 1;
        TextureDesc.ArraySize               = 1;
        TextureDesc.Format                  = DXGI_FORMAT_R32G32B32A32_FLOAT;
        TextureDesc.SampleDesc.Count        = 1;
        TextureDesc.Usage                   = D3D11_USAGE_DEFAULT;
        TextureDesc.BindFlags               = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        TextureDesc.CPUAccessFlags          = 0;
        TextureDesc.MiscFlags               = 0;

        hResult = Device->CreateTexture2D(&TextureDesc, nullptr, HeatmapTexture.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create heatmap buffer");
    }

    // --- Create Shader Resource View for HeatmapBuffer ---
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.Format                          = DXGI_FORMAT_R32G32B32A32_FLOAT;
        SRVDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels             = 1;
        SRVDesc.Texture2D.MostDetailedMip       = 0;

        hResult = Device->CreateShaderResourceView(HeatmapTexture.Get(), &SRVDesc, HeatmapTextureSRV.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create heatmap buffer SRV");
    }

    // --- Create Unordered Access View for HeatmapBuffer ---
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc    = {};
        UAVDesc.Format                              = DXGI_FORMAT_R32G32B32A32_FLOAT;
        UAVDesc.ViewDimension                       = D3D11_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Texture2D.MipSlice                  = 0;

        hResult = Device->CreateUnorderedAccessView(HeatmapTexture.Get(), &UAVDesc, HeatmapTextureUAV.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create heatmap buffer UAV");
    }
}

void FTileLightManager::CreateConstantBuffer()
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11Device* Device = Renderer->GetRHIDevice()->GetDevice();
    HRESULT hResult;

    // --- Create CameraInfoConstantBuffer ---
    {
        D3D11_BUFFER_DESC BufferDesc    = {};
        BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
        BufferDesc.ByteWidth            = sizeof(FCameraBufferType);
        BufferDesc.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
        BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags            = 0;
        BufferDesc.StructureByteStride  = 0;

        hResult = Device->CreateBuffer(&BufferDesc, nullptr, CameraInfoConstantBuffer.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create camera info constant buffer");
    }

    // --- Create ViewportConstantBuffer ---
    {
        D3D11_BUFFER_DESC BufferDesc    = {};
        BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
        BufferDesc.ByteWidth            = sizeof(ViewportBufferType);
        BufferDesc.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
        BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags            = 0;
        BufferDesc.StructureByteStride  = 0;

        hResult = Device->CreateBuffer(&BufferDesc, nullptr, ViewportConstantBuffer.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create viewport constant buffer");
    }
    
    // --- Create TileConstantBuffer ---
    {
        D3D11_BUFFER_DESC BufferDesc    = {};
        BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
        BufferDesc.ByteWidth            = sizeof(FTileBufferType);
        BufferDesc.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
        BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags            = 0;
        BufferDesc.StructureByteStride  = 0;

        hResult = Device->CreateBuffer(&BufferDesc, nullptr, TileConstantBuffer.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create tile constant buffer");
    }
    
    // --- Create LightingConstantBuffer ---
    {
        D3D11_BUFFER_DESC BufferDesc    = {};
        BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
        BufferDesc.ByteWidth            = sizeof(FLightingBufferType);
        BufferDesc.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
        BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags            = 0;
        BufferDesc.StructureByteStride  = 0;

        hResult = Device->CreateBuffer(&BufferDesc, nullptr, LightingConstantBuffer.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create tile constant buffer");
    }

    // --- Create SliceConstantBuffer ---
    {
        D3D11_BUFFER_DESC BufferDesc    = {};
        BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
        BufferDesc.ByteWidth            = sizeof(FSliceInfoBufferType);
        BufferDesc.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
        BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags            = 0;
        BufferDesc.StructureByteStride  = 0;

        hResult = Device->CreateBuffer(&BufferDesc, nullptr, SliceConstantBuffer.ReleaseAndGetAddressOf());
        assert(SUCCEEDED(hResult) && "Failed to create slice constant buffer");
    }
}

void FTileLightManager::UpdateConstantBuffer()
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11DeviceContext* DeviceContext = Renderer->GetRHIDevice()->GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    DeviceContext->Map(TileConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    FTileBufferType* Tile = static_cast<FTileBufferType*>(MappedResource.pData);
    Tile->NumGroupsX = (Width + TILE_WIDTH - 1) / TILE_WIDTH;
    Tile->NumGroupsY = (Height + TILE_HEIGHT - 1) / TILE_HEIGHT;
    DeviceContext->Unmap(TileConstantBuffer.Get(), 0);
}

void FTileLightManager::UpdateConstantBuffer(UCameraComponent* InCameraComponent, float InAspectRatio)
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11DeviceContext* DeviceContext = Renderer->GetRHIDevice()->GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    DeviceContext->Map(CameraInfoConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    FCameraBufferType* CameraInfo = static_cast<FCameraBufferType*>(MappedResource.pData);
    CameraInfo->ViewMatrix = InCameraComponent->GetViewMatrix();
    CameraInfo->ProjectionMatrix = InCameraComponent->GetProjectionMatrix(InAspectRatio);
    CameraInfo->InverseViewMatrix = InCameraComponent->GetViewMatrix().Inverse();
    CameraInfo->InverseProjectionMatrix = InCameraComponent->GetProjectionMatrix(InAspectRatio).Inverse();
    CameraInfo->NearClip = InCameraComponent->GetNearClip();
    CameraInfo->FarClip = InCameraComponent->GetFarClip();
    DeviceContext->Unmap(CameraInfoConstantBuffer.Get(), 0);
}

void FTileLightManager::UpdateConstantBuffer(FViewport* InViewport)
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11DeviceContext* DeviceContext = Renderer->GetRHIDevice()->GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    DeviceContext->Map(ViewportConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    ViewportBufferType* Viewport = static_cast<ViewportBufferType*>(MappedResource.pData);
    Viewport->ViewportRect = FVector4(
        static_cast<float>(InViewport->GetStartX()),
        static_cast<float>(InViewport->GetStartY()),
        static_cast<float>(InViewport->GetSizeX()),
        static_cast<float>(InViewport->GetSizeY()));
    DeviceContext->Unmap(ViewportConstantBuffer.Get(), 0);
}

void FTileLightManager::UpdateConstantBuffer(FLightingBufferType& LightingBuffer)
{
    assert(Renderer && "Initialization not done: Renderer is null");

    ID3D11DeviceContext* DeviceContext = Renderer->GetRHIDevice()->GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    DeviceContext->Map(LightingConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    memcpy(MappedResource.pData, &LightingBuffer, sizeof(LightingBuffer)); 
    DeviceContext->Unmap(LightingConstantBuffer.Get(), 0);
}
