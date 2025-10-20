/** @note These macro values should correspond with values defined in FTileLightManager class. */
/** @todo Substitute hard-coded macro definitions by passing values from CPU side. */
#define TILE_WIDTH              32
#define TILE_HEIGHT             32
#define NUM_SLICES              32
#define BUCKET_SIZE             32
#define NUM_LIGHT_PER_BUCKET    32

/*-----------------------------------------------------------------------------
    Structs
 -----------------------------------------------------------------------------*/

struct FPointLightInfo
{
    float3 Color;               // light color
    float3 Position;            // world space position
    float intensity;
    float AttenuationRadius;
    float LightFalloffExponent; // exponent
    float1 Pad; 
};

struct FSphere
{
    float3 Position;
    
    float Radius;
};

struct FPlane
{
    float3 Normal;
    
    float Distance;
};

struct FFrustum
{
    FPlane Planes[6];
};

/*-----------------------------------------------------------------------------
    Constant Buffers
 -----------------------------------------------------------------------------*/

cbuffer CameraBuffer : register(b0)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
    float NearClip;
    float FarClip;
    float2 _Pad_0;
}

cbuffer ViewportBuffer : register(b1)
{
    float4 ViewportRect; // x=StartX, y=StartY, z=Width, w=Height
}

cbuffer LightBuffer : register(b2)
{
    uint NumPointLights;
    
    uint NumSpotLights;
}

// cbuffer SliceBuffer : register(b3)
// {
//     float SliceScale;
//     
//     float SliceBias;
// }

/*-----------------------------------------------------------------------------
    GPU Resources
 -----------------------------------------------------------------------------*/

Texture2D<float> DepthTexture : register(t0);

StructuredBuffer<FPointLightInfo> PointLights : register(t1);

// StructuredBuffer<FSpotLightInfo> SpotLights : register(t2);

RWStructuredBuffer<uint> PointLightMask : register(u0);

// RWStructuredBuffer<uint> SpotLightMask : register(u1);

RWTexture2D<float4> HeatmapTexture : register(u2);

groupshared uint TileDepthMask;
groupshared uint MinDepth;
groupshared uint MaxDepth;
groupshared uint VisibleLightCount;
// groupshared uint VisibleLightCache[MAX_NUM_LIGHT_PER_TILE];

/*-----------------------------------------------------------------------------
    Helper Functions
 -----------------------------------------------------------------------------*/

FPlane CreatePlane(float3 Point0, float3 Point1, float3 Point2)
{
    FPlane Plane;
    Plane.Normal = normalize(cross(Point1 - Point0, Point2 - Point0));
    Plane.Distance = dot(Plane.Normal, Point0);
    return Plane;
}

FFrustum CreateViewFrustum(float2 TileMin, float2 TileMax, float Width, float Height)
{
    float2 NDCMin = (TileMin / float2(Width, Height)) * 2.0f - 1.0f;
    float2 NDCMax = (TileMax / float2(Width, Height)) * 2.0f - 1.0f;
    
    NDCMin.y = -NDCMin.y;
    NDCMax.y = -NDCMax.y;

    // NDC to View Space Coordinate
    float4 ViewCorners[4];
    ViewCorners[0] = mul(float4(NDCMin.x, NDCMin.y, 1.0f, 1.0f), InverseProjectionMatrix); // Bottom Left
    ViewCorners[1] = mul(float4(NDCMax.x, NDCMin.y, 1.0f, 1.0f), InverseProjectionMatrix); // Bottom Right
    ViewCorners[2] = mul(float4(NDCMin.x, NDCMax.y, 1.0f, 1.0f), InverseProjectionMatrix); // Top Left
    ViewCorners[3] = mul(float4(NDCMin.x, NDCMin.y, 1.0f, 1.0f), InverseProjectionMatrix); // Top Right

    // --- Perspective Division ---
    ViewCorners[0] /= ViewCorners[0].w;
    ViewCorners[1] /= ViewCorners[1].w;
    ViewCorners[2] /= ViewCorners[2].w;
    ViewCorners[3] /= ViewCorners[3].w;

    // --- Create View Frustum ---
    FFrustum Frustum;
    Frustum.Planes[0] = CreatePlane(float3(0.0f, 0.0f, 0.0f), ViewCorners[0].xyz, ViewCorners[1].xyz); // Bottom Plane
    Frustum.Planes[1] = CreatePlane(float3(0.0f, 0.0f, 0.0f), ViewCorners[1].xyz, ViewCorners[3].xyz); // Right Plane
    Frustum.Planes[2] = CreatePlane(float3(0.0f, 0.0f, 0.0f), ViewCorners[3].xyz, ViewCorners[2].xyz); // Top Plane
    Frustum.Planes[3] = CreatePlane(float3(0.0f, 0.0f, 0.0f), ViewCorners[2].xyz, ViewCorners[0].xyz); // Left Plane
    Frustum.Planes[4].Normal = float3(0.0f, 0.0f, -1.0f); // Near Plane
    Frustum.Planes[4].Distance = NearClip;
    Frustum.Planes[5].Normal = float3(0.0f, 0.0f, 1.0f); // Far Plane
    Frustum.Planes[5].Distance = FarClip;

    return Frustum;
}

bool IsSphereInsidePlane(FSphere Sphere, FPlane Plane)
{
    return dot(Sphere.Position, Plane.Normal) - Plane.Distance < Sphere.Radius;
}

bool IsSphereInsideFrustum(FSphere Sphere, FFrustum Frustum)
{
    for (uint i = 0; i < 6; ++i)
    {
        if (!IsSphereInsidePlane(Sphere, Frustum.Planes[i]))
        {
            return false;
        }
    }
    return true;
}

void CullPointLight(uint Index, FSphere Sphere, FFrustum Frustum)
{
    if (!IsSphereInsideFrustum(Sphere, Frustum))
    {
        return; 
    }

    uint BucketIndex = Index / BUCKET_SIZE;
    uint BitIndex = Index % BUCKET_SIZE;
    InterlockedOr(PointLightMask[BUCKET_SIZE + BucketIndex], 1u << BitIndex);
    InterlockedAdd(VisibleLightCount, 1u);
}

void VisualizeDepthSlices(uint InTileDepthMask, uint2 InPixelCoord, RWTexture2D<float4> OutHeatmapTexture)
{
    // --- Depth Clustering Test ---
    float R = pow((InTileDepthMask & 0xF) / 255.0f, 1.0f / 2.2f);
    float G = pow(((InTileDepthMask >> 4) & 0xF) / 255.0f, 1.0f / 2.2f);
    float B = pow(((InTileDepthMask >> 8) & 0xF) / 255.0f, 1.0f / 2.2f);
    float4 HeatmapColor = float4(R, G, B, 1.0f);

    if (InPixelCoord.x >= ViewportRect.x && InPixelCoord.x < ViewportRect.x + ViewportRect.z &&
        InPixelCoord.y >= ViewportRect.y && InPixelCoord.y < ViewportRect.y + ViewportRect.w)
    {
        OutHeatmapTexture[InPixelCoord] = HeatmapColor;
    }
}

void VisualizeLightCount(uint InVisibleLightCount, uint2 InPixelCoord, RWTexture2D<float4> OutHeatmapTexture)
{
    // --- Light Count Heatmap Visualization ---
    
    const float MAX_LIGHTS_FOR_HEATMAP = 64.0f;

    float HeatIntensity = saturate((float)InVisibleLightCount / MAX_LIGHTS_FOR_HEATMAP);

    float4 HeatmapColor;

    if (HeatIntensity < 0.5f)
    {
        HeatmapColor = lerp(float4(0.0f, 0.0f, 1.0f, 1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f), HeatIntensity * 2.0f);
    }
    else
    {
        HeatmapColor = lerp(float4(0.0f, 1.0f, 0.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f), (HeatIntensity - 0.5f) * 2.0f);
    }

    if (InPixelCoord.x >= ViewportRect.x && InPixelCoord.x < ViewportRect.x + ViewportRect.z &&
        InPixelCoord.y >= ViewportRect.y && InPixelCoord.y < ViewportRect.y + ViewportRect.w)
    {
        OutHeatmapTexture[InPixelCoord] = HeatmapColor;
    }
}

/*-----------------------------------------------------------------------------
    Main Function
 -----------------------------------------------------------------------------*/

[numthreads(TILE_WIDTH, TILE_HEIGHT, 1)]
void mainCS(uint3 GroupID : SV_GroupID, uint3 ThreadID : SV_GroupThreadID, uint3 DispatchID : SV_DispatchThreadID)
{
    if (ThreadID.x == 0 && ThreadID.y == 0)
    {
        TileDepthMask = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint2 PixelCoord = GroupID.xy * uint2(TILE_WIDTH, TILE_HEIGHT) + ThreadID.xy;

    if (PixelCoord.x >= ViewportRect.x && PixelCoord.x < ViewportRect.x + ViewportRect.z &&
        PixelCoord.y >= ViewportRect.y && PixelCoord.y < ViewportRect.y + ViewportRect.w)
    {
        float DepthSample = DepthTexture[PixelCoord];

        if (DepthSample < 1.0f)
        {
            float LinearDepth = (NearClip * FarClip) / (FarClip - DepthSample * (FarClip - NearClip));

            float NormalizedDepth = saturate((LinearDepth - NearClip) / (FarClip - NearClip));

            uint SliceIndex = (uint)(floor(NormalizedDepth * NUM_SLICES));
            SliceIndex = clamp(SliceIndex, 0, NUM_SLICES - 1);
            
            InterlockedMin(MinDepth, uint(LinearDepth));
            InterlockedMax(MaxDepth, uint(LinearDepth));
            InterlockedOr(TileDepthMask, 1u << SliceIndex);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    if (ThreadID.x == 0 && ThreadID.y == 0)
    {
        float2 TileMin = GroupID.xy * float2(TILE_WIDTH, TILE_HEIGHT);
        float2 TileMax = TileMin + float2(TILE_WIDTH, TILE_HEIGHT);

        float Width;
        float Height;
        DepthTexture.GetDimensions(Width, Height);
    
        FFrustum Frustum = CreateViewFrustum(TileMin, TileMax, Width, Height);
        for (uint i = 0; i < NumPointLights; ++i)
        {
            FSphere Sphere = {};
            Sphere.Position = mul(float4(PointLights[i].Position, 1.0f), ViewMatrix);
            Sphere.Radius = PointLights[i].AttenuationRadius;

            CullPointLight(i, Sphere, Frustum);
        }
    }
    
    // --- Depth Clustering Test ---
    // VisualizeDepthSlices(TileDepthMask, PixelCoord, HeatmapTexture);
    
    // --- Light Count Heatmap Visualization ---
    VisualizeLightCount(VisibleLightCount, PixelCoord, HeatmapTexture);
}