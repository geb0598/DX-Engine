struct FAABB
{
	float3 Min;
	float Pad0;
	float3 Max;
	float Pad1;
};

Texture2D<float> HiZTexture : register(t0);

StructuredBuffer<FAABB> NDCAABBs : register(t1);

RWStructuredBuffer<uint> Visibilities : register(u0);

cbuffer OcclusionCullingConstants : register(b0)
{
	uint NumAABBs;
	uint MipLevels;
	float2 ScreenSize;
}

SamplerState OcclusionCullingSampler : register(s0);

uint CalculateMipLevel(uint InWidth, uint InHeight)
{
	return (int)ceil(log2(max(InWidth, InHeight)));
}

[numthreads(64, 1, 1)]
void mainCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint Index = DispatchThreadID.x;

	if (Index >= NumAABBs)
	{
		return;
	}

	float3 MinNDC = NDCAABBs[Index].Min;
	float3 MaxNDC = NDCAABBs[Index].Max;

	float MinDepth = MinNDC.z;

	float2 MinUV;
	MinUV.x = MinNDC.x * 0.5f + 0.5f;
	MinUV.y = MinNDC.y * -0.5f + 0.5f;
	MinUV = saturate(MinUV);

	float2 MaxUV;
	MaxUV.x = MaxNDC.x * 0.5f + 0.5f;
	MaxUV.y = MaxNDC.y * -0.5f + 0.5f;
	MaxUV = saturate(MaxUV);

	float2 Rect = (MaxUV - MinUV) * ScreenSize;

	uint MipLevel = CalculateMipLevel(Rect.x, Rect.y);

	float HiZDepth0 = HiZTexture.SampleLevel(OcclusionCullingSampler, float2(MinUV.x, MinUV.y), MipLevel);
	float HiZDepth1 = HiZTexture.SampleLevel(OcclusionCullingSampler, float2(MaxUV.x, MinUV.y), MipLevel);
	float HiZDepth2 = HiZTexture.SampleLevel(OcclusionCullingSampler, float2(MinUV.x, MaxUV.y), MipLevel);
	float HiZDepth3 = HiZTexture.SampleLevel(OcclusionCullingSampler, float2(MaxUV.x, MaxUV.y), MipLevel);

	float MaxHiZDepth = max(max(HiZDepth0, HiZDepth1), max(HiZDepth2, HiZDepth3));

	float Bias = 0.0001f;

	bool bIsVisible = MinDepth <= MaxHiZDepth + Bias;

	Visibilities[Index] = bIsVisible;
}
