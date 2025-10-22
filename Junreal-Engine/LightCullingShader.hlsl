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

struct FAmbientLightInfo
{
    float4 Color; // light color
    float Intensity; 
    float3 Pad0;
};

struct FDirectionalLightInfo
{
    float4 Color; // light color
    float3 Direction;  // world space direction
    float Intensity; 
};

struct FPointLightInfo
{
    float4 Color;               // light color
    float3 Position;            // world space position
    float intensity;
    float AttenuationRadius;
    float LightFalloffExponent; // exponent
    float2 Pad; 
};

struct FSpotLightInfo
{
    float4 Color; // light color
    float3 Position; // world space position
    float Intensity;
    float3 Direction; // world space direction
    float AttenuationRadius;
    float InnerConeAngle; // cos
    float OuterConeAngle; // cos
    float LightFalloffExponent; // exponent
    float Pad0;
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

struct FCone
{
    // 0 -> Position of Apex
    // 1-4 -> Vertex Positions of Bounding Square of a Circle
    float3 Positions[5];
};

/*-----------------------------------------------------------------------------
    Constant Buffers
 -----------------------------------------------------------------------------*/

cbuffer Camera : register(b0)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
    float NearClip;
    float FarClip;
    float2 _Pad_0;
}

cbuffer Viewport : register(b1)
{
    float4 ViewportRect; // x=StartX, y=StartY, z=Width, w=Height
}

cbuffer Tile : register(b2)
{
    uint NumGroupsX;
    uint NumGroupsY;
}

cbuffer Lighting : register(b3)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    float3 CameraPos; // world space camera position
    uint NumPointLights;
    uint NumSpotLights;
    float3 Pad1;
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

StructuredBuffer<FPointLightInfo> PointLights : register(t2);

StructuredBuffer<FSpotLightInfo> SpotLights : register(t3);

RWStructuredBuffer<uint> PointLightMask : register(u0);

RWStructuredBuffer<uint> SpotLightMask : register(u1);

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

FFrustum CreateViewFrustum(float2 TileMin, float2 TileMax, float4 ViewportRect)
{
    float2 ViewportMin = TileMin - ViewportRect.xy;
    float2 ViewportMax = TileMax - ViewportRect.xy;
    
    float2 NDCMin = (ViewportMin / ViewportRect.zw) * 2.0f - 1.0f;
    float2 NDCMax = (ViewportMax / ViewportRect.zw) * 2.0f - 1.0f;
    
    NDCMin.y = -NDCMin.y;
    NDCMax.y = -NDCMax.y;

    // NDC to View Space Coordinate
    float4 ViewCorners[4];
    ViewCorners[0] = mul(float4(NDCMin.x, NDCMin.y, 1.0f, 1.0f), InverseProjectionMatrix); // Bottom Left
    ViewCorners[1] = mul(float4(NDCMax.x, NDCMin.y, 1.0f, 1.0f), InverseProjectionMatrix); // Bottom Right
    ViewCorners[2] = mul(float4(NDCMin.x, NDCMax.y, 1.0f, 1.0f), InverseProjectionMatrix); // Top Left
    ViewCorners[3] = mul(float4(NDCMax.x, NDCMax.y, 1.0f, 1.0f), InverseProjectionMatrix); // Top Right

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

uint SphereToDepthMask(FSphere SphereVS)
{
    float SphereMinDepth = SphereVS.Position.z - SphereVS.Radius;
    float SphereMaxDepth = SphereVS.Position.z + SphereVS.Radius;
    float NormalizedSphereMinDepth = saturate((SphereMinDepth - NearClip) / (FarClip - NearClip));
    float NormalizedSphereMaxDepth = saturate((SphereMaxDepth - NearClip) / (FarClip - NearClip));
    float MinSliceIndex = (uint)(floor(NormalizedSphereMinDepth * NUM_SLICES));
    MinSliceIndex = clamp(MinSliceIndex, 0, NUM_SLICES - 1);
    float MaxSliceIndex = (uint)(ceil(NormalizedSphereMaxDepth * NUM_SLICES));
    MaxSliceIndex = clamp(MaxSliceIndex, 0, NUM_SLICES - 1);
    
    uint DepthMask = 0u;
    for (int i = MinSliceIndex; i <= MaxSliceIndex; ++i)
    {
        DepthMask |= (1u << i);
    }

    return DepthMask;
}

void CullPointLight(uint Index, uint FlatTileIndex, FSphere SphereVS, FFrustum Frustum)
{
    if (!IsSphereInsideFrustum(SphereVS, Frustum))
    {
        return; 
    }

    if (!(TileDepthMask & SphereToDepthMask(SphereVS)))
    {
        return; 
    }

    uint BucketIndex = Index / BUCKET_SIZE;
    uint BitIndex = Index % BUCKET_SIZE;
    InterlockedOr(PointLightMask[FlatTileIndex * BUCKET_SIZE + BucketIndex], 1u << BitIndex);
    InterlockedAdd(VisibleLightCount, 1u);
}

FCone CreateCone(float3 ApexPosition, float3 AxisDirection, float Height, float CosAngle)
{
    FCone Cone;
    Cone.Positions[0] = ApexPosition;

    AxisDirection = normalize(AxisDirection);
    float3 BaseCenterPosition = ApexPosition + AxisDirection * Height;

    float BaseRadius = Height * tan(acos(CosAngle));

    float3 BasisRerenceUp;
    if (abs(dot(AxisDirection, float3(0.0f, 0.0f, 1.0f))) > 0.999f)
    {
        BasisRerenceUp = float3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        BasisRerenceUp = float3(0.0f, 1.0f, 0.0f);
    }

    float3 BasePlaneRight = normalize(cross(AxisDirection, BasisRerenceUp));
    float3 BasePlaneForward = normalize(cross(BasePlaneRight, AxisDirection));

    Cone.Positions[1] = BaseCenterPosition + BasePlaneForward * BaseRadius;
    Cone.Positions[2] = BaseCenterPosition - BasePlaneRight * BaseRadius;
    Cone.Positions[3] = BaseCenterPosition + BasePlaneForward * BaseRadius;
    Cone.Positions[4] = BaseCenterPosition - BasePlaneForward * BaseRadius;

    return Cone;
}

bool IsConeInsidePlane(FCone Cone, FPlane Plane)
{
    for (int i = 0; i < 5; ++i)
    {
        float SignedDistance = dot(Cone.Positions[i], normalize(Plane.Normal)) - Plane.Distance;
        if (SignedDistance <= 0.0f)
        {
            return true;
        }
    }
    return false;
}

bool IsConeInsideFrustum(FCone Cone, FFrustum Frustum)
{
    for (int i = 0; i < 6; ++i)
    {
        if (!IsConeInsidePlane(Cone, Frustum.Planes[i]))
        {
            return false;
        }
    }
    return true;
}

uint ConeToDepthMask(FCone ConeVS)
{
    float MinDepth = ConeVS.Positions[0].z; 
    float MaxDepth = ConeVS.Positions[0].z;

    for (int i = 1; i < 5; ++i)
    {
        MinDepth = min(MinDepth, ConeVS.Positions[i].z);
        MaxDepth = max(MaxDepth, ConeVS.Positions[i].z);
    }

    float NormalizedMinDepth = saturate((MinDepth - NearClip) / (FarClip - NearClip));
    float NormalizedMaxDepth = saturate((MaxDepth - NearClip) / (FarClip - NearClip));

    uint MinSliceIndex = (uint)(floor(NormalizedMinDepth * NUM_SLICES));
    MinSliceIndex = clamp(MinSliceIndex, 0, NUM_SLICES - 1);
    
    uint MaxSliceIndex = (uint)(ceil(NormalizedMaxDepth * NUM_SLICES));
    MaxSliceIndex = clamp(MaxSliceIndex, 0, NUM_SLICES - 1);
    
    uint DepthMask = 0u;
    for (uint i = MinSliceIndex; i <= MaxSliceIndex; ++i)
    {
        DepthMask |= (1u << i);
    }

    return DepthMask;
}

void CullSpotLight(uint Index, uint FlatTileIndex, FSphere Sphere, FFrustum Frustum)
{
    if (!IsSphereInsideFrustum(Sphere, Frustum))
    {
        return;
    }

    if (!(TileDepthMask & SphereToDepthMask(Sphere)))
    {
        return;
    }

    uint BucketIndex = Index / BUCKET_SIZE;
    uint BitIndex = Index % BUCKET_SIZE;
    InterlockedOr(SpotLightMask[FlatTileIndex * BUCKET_SIZE + BucketIndex], 1u << BitIndex);
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
    if (InPixelCoord.x < ViewportRect.x || InPixelCoord.x >= ViewportRect.x + ViewportRect.z ||
        InPixelCoord.y < ViewportRect.y || InPixelCoord.y >= ViewportRect.y + ViewportRect.w)
    {
        return;
    }
    
    float4 HeatmapColor;

    if (InVisibleLightCount == 0)
    {
        HeatmapColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        const float MAX_LIGHTS_FOR_HEATMAP = 10.0f;
        float HeatIntensity = saturate((float)InVisibleLightCount / MAX_LIGHTS_FOR_HEATMAP);

        const float4 ColdColor = float4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
        const float4 CoolColor = float4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
        const float4 MidColor  = float4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        const float4 WarmColor = float4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        const float4 HotColor  = float4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        
        if (HeatIntensity < 0.25f)
        {
            HeatmapColor = lerp(ColdColor, CoolColor, HeatIntensity * 4.0f);
        }
        else if (HeatIntensity < 0.5f)
        {
            HeatmapColor = lerp(CoolColor, MidColor, (HeatIntensity - 0.25f) * 4.0f);
        }
        else if (HeatIntensity < 0.75f)
        {
            HeatmapColor = lerp(MidColor, WarmColor, (HeatIntensity - 0.5f) * 4.0f);
        }
        else
        {
            HeatmapColor = lerp(WarmColor, HotColor, (HeatIntensity - 0.75f) * 4.0f);
        }
    }

    OutHeatmapTexture[InPixelCoord] = HeatmapColor;
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
        MinDepth = 0xFFFFFFFF;
        MaxDepth = 0;
        VisibleLightCount = 0;
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
        uint FlatTileIndex = GroupID.x + GroupID.y * NumGroupsX;
        float2 TileMin = GroupID.xy * float2(TILE_WIDTH, TILE_HEIGHT);
        float2 TileMax = TileMin + float2(TILE_WIDTH, TILE_HEIGHT);
    
        FFrustum Frustum = CreateViewFrustum(TileMin, TileMax, ViewportRect);
        for (uint i = 0; i < NumPointLights; ++i)
        {
            FSphere Sphere;
            Sphere.Position = mul(float4(PointLights[i].Position, 1.0f), ViewMatrix);
            Sphere.Radius = PointLights[i].AttenuationRadius;
    
            CullPointLight(i, FlatTileIndex, Sphere, Frustum);
        }

        for (uint i = 0; i < NumSpotLights; ++i)
        {
            FSphere Sphere;
            Sphere.Position = mul(float4(SpotLights[i].Position, 1.0f), ViewMatrix);
            Sphere.Radius = SpotLights[i].AttenuationRadius;
            CullSpotLight(i, FlatTileIndex, Sphere, Frustum);
        }
    }

    // --- Depth Clustering Test ---
    // VisualizeDepthSlices(TileDepthMask, PixelCoord, HeatmapTexture);

    GroupMemoryBarrierWithGroupSync();
    
    // --- Light Count Heatmap Visualization ---
    VisualizeLightCount(VisibleLightCount, PixelCoord, HeatmapTexture);
}