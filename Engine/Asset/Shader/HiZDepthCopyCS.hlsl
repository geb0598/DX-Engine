Texture2D<float> DepthTexture : register(t0);

RWTexture2D<float> HiZTexture : register(u0);

[numthreads(16, 16, 1)]
void mainCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint Width; uint Height;
	HiZTexture.GetDimensions(Width, Height);

	if (DispatchThreadID.x >= Width || DispatchThreadID.y >= Height)
	{
		return;
	}

	HiZTexture[DispatchThreadID.xy] = DepthTexture.Load(int3(DispatchThreadID.xy, 0));
}
