/** @note These macro values should correspond with values defined in FTileLightManager class. */
#define TILE_WIDTH  32
#define TILE_HEIGHT 32
#define NUM_SLICES  32

// struct FPointLightInfo
// {
//     float4 Color; // light color
//     float3 Position; // world space position
//     float intensity;
//     float AttenuationRadius;
//     float LightFalloffExponent; // exponent
//     float2 Pad0; 
// };

cbuffer CameraInfoBuffer : register(b0)
{
    float NearClip;
    
    float FarClip;
}

cbuffer ViewportBuffer : register(b1)
{
    float4 ViewportRect; // x=StartX, y=StartY, z=Width, w=Height
}

// cbuffer SliceBuffer : register(b2)
// {
//     float SliceScale;
//     
//     float SliceBias;
// }

Texture2D<float> DepthTexture : register(t0);

// RWStructuredBuffer<uint> PointLightMask : register(u0);

/** @todo Change slot to 1 */
RWTexture2D<float4> HeatmapTexture : register(u0);

groupshared uint TileDepthMask;
// groupshared uint MinDepthInt;
// groupshared uint MaxDepthInt;
// groupshared uint VisibleLightCount;
// groupshared uint VisibleLightCache[MAX_NUM_LIGHT_PER_TILE];

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
            InterlockedOr(TileDepthMask, 1u << SliceIndex);
        }
    }
    GroupMemoryBarrierWithGroupSync();

    uint NumBits = countbits(TileDepthMask);

    float NormalizedNumBits = NumBits / (float)NUM_SLICES;
    NormalizedNumBits = saturate(NormalizedNumBits * 10.0f);

    float4 HeatmapColor = lerp(float4(0.0f, 0.0f, 1.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f), NormalizedNumBits);

    if (PixelCoord.x >= ViewportRect.x && PixelCoord.x < ViewportRect.x + ViewportRect.z &&
        PixelCoord.y >= ViewportRect.y && PixelCoord.y < ViewportRect.y + ViewportRect.w)
    {
        HeatmapTexture[PixelCoord] = HeatmapColor;
    }
}