#pragma once

#include <wrl.h>

#include <d3d11.h>

class FViewport;
class UCameraComponent;
class UPointLightComponent;
class URenderer;
class USpotLightComponent;

/*-----------------------------------------------------------------------------
    FTileLightManager
 -----------------------------------------------------------------------------*/

/**
 * Singleton class to support 2.5D Tilemap light culling
 * It manages lifecycle of resources related with light culling by itself.
 * It is too redundant to expand RHIDevice related classes for resources that wouldn't be recycled.
 * Also, existing rendering resource related classes wouldn't satisfy requirements of this class.
 * So, it solely keeps its resources until they are shared by other parts of this project or rendering resource management satisfies its requirements.
 */
class FTileLightManager
{
public:
    static FTileLightManager& GetInstance()
    {
        static FTileLightManager Instance;
        return Instance;
    }

    /** @note Should be invoked before using */
    void Initialize(URenderer* InRenderer);

    bool IsInitialized() const { return !!(Renderer); }

    void OnResize(uint32 InWidth, uint32 InHeight);
    
    /*-----------------------------------------------------------------------------
        Tile-based 2.5D Light Culling
     -----------------------------------------------------------------------------*/

    void CullPointLights(UCameraComponent* InCameraComponent, FViewport* InViewport, FLightingBufferType& LightingBuffer);

    void CullSpotLights(UCameraComponent* InCameraComponent, FViewport* Viewport) {}
    
    /*-----------------------------------------------------------------------------
        Getters / Setters
     -----------------------------------------------------------------------------*/

    ID3D11ShaderResourceView* GetPointLightMaskBufferSRV() const { return PointLightMaskBufferSRV.Get(); }

    ID3D11ShaderResourceView* GetSpotLightMaskBufferSRV() const { return SpotLightMaskBufferSRV.Get(); }
    
    /*-----------------------------------------------------------------------------
        Debug / Visualization Features
     -----------------------------------------------------------------------------*/
public:
    void RenderPointLightHeatmap();

    /** @note currently it sets viewport and restore it to make it compatible with texture size of heatmap.
     *  @todo Revise it to make it consistent with postprocess logic.
     */
    void RenderSpotLightHeatmap() {}

    uint32 GetCulledPointLightsCount() const { return 0; }

    uint32 GetCulledSpotLightsCount() const { return 0; }

private:
    // --- Tile information ---

    /** @note This value should correspond with numthreads(TILE_WIDTH, _, _) */
    static constexpr uint32 TILE_WIDTH = 32;
    
    /** @note This value should correspond with numthreads(_, TILE_HEIGHT, _) */
    static constexpr uint32 TILE_HEIGHT = 32;

    static constexpr uint32 NUM_SLICES = 32;

    // --- Light information ---
    
    using BucketType = uint32;
    
    static constexpr uint32 BUCKET_SIZE = sizeof(BucketType) * 8u;
    
    static constexpr uint32 MAX_NUM_LIGHT_PER_TILE = 1024;

    static constexpr uint32 NUM_LIGHT_PER_BUCKET = MAX_NUM_LIGHT_PER_TILE / BUCKET_SIZE;

    /*-----------------------------------------------------------------------------
        GPU Resources
     -----------------------------------------------------------------------------*/
private:
    void CreateShader(const FString& InFilePath, const FString& InEntryPoint);
    
    void CreateBuffer();

    void CreatePointLightBuffer();

    void CreatePointLightMaskBuffer();
    
    void CreateSpotLightBuffer() {}

    void CreateSpotLightMaskBuffer() {}

    void CreateConstantBuffer();

    /** @note Update tile information */
    void UpdateConstantBuffer();

    void UpdateConstantBuffer(UCameraComponent* InCameraComponent, float InAspectRatio);

    void UpdateConstantBuffer(FViewport* InViewport);
    
    void UpdateConstantBuffer(UPointLightComponent* PointLightComponent) {}

    void UpdateConstantBuffer(FLightingBufferType& LightingBuffer);
    
    void CreateHeatmapTexture();
    
    URenderer* Renderer;

    uint32 Width;
    uint32 Height;

    // --- Shaders ---
    
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> ComputeShader;
    
    // --- Buffers ---

    Microsoft::WRL::ComPtr<ID3D11Buffer> PointLightBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PointLightBufferSRV;

    Microsoft::WRL::ComPtr<ID3D11Buffer> PointLightMaskBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PointLightMaskBufferSRV;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> PointLightMaskBufferUAV;
    
    Microsoft::WRL::ComPtr<ID3D11Buffer> SpotLightBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SpotLightBufferSRV;

    Microsoft::WRL::ComPtr<ID3D11Buffer> SpotLightMaskBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SpotLightMaskBufferSRV;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> SpotLightMaskBufferUAV;

    // --- Textures ---

    Microsoft::WRL::ComPtr<ID3D11Texture2D> HeatmapTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> HeatmapTextureSRV;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> HeatmapTextureUAV;

    // --- Constant Buffers ---

    Microsoft::WRL::ComPtr<ID3D11Buffer> CameraInfoConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> ViewportConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> TileConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> LightingConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> SliceConstantBuffer;
    
    /*-----------------------------------------------------------------------------
        FTileLightManager Lifecycles (Singleton)
     -----------------------------------------------------------------------------*/
private:
    FTileLightManager() = default;

    ~FTileLightManager() = default;

    FTileLightManager(const FTileLightManager&) = delete;
    FTileLightManager& operator=(const FTileLightManager&) = delete;
    
    FTileLightManager(FTileLightManager&&) = delete;
    FTileLightManager& operator=(FTileLightManager&&) = delete;
};