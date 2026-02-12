Texture2D<float> InputHiZTexture : register(t0);

RWTexture2D<float> OutputHiZTexture : register(u0);

[numthreads(16, 16, 1)]
void mainCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint OutputWidth;
	uint OutputHeight;
	OutputHiZTexture.GetDimensions(OutputWidth, OutputHeight);

	if (DispatchThreadID.x >= OutputWidth || DispatchThreadID.y >= OutputHeight)
	{
		return;
	}

	uint InputWidth;
	uint InputHeight;
	InputHiZTexture.GetDimensions(InputWidth, InputHeight);

	uint2 SourceCoord = DispatchThreadID * 2;

	float Depth0 = InputHiZTexture.Load(int3(SourceCoord + uint2(0, 0), 0));
	float Depth1 = InputHiZTexture.Load(int3(min(SourceCoord + uint2(1, 0), uint2(InputWidth - 1, InputHeight - 1)), 0));
	float Depth2 = InputHiZTexture.Load(int3(min(SourceCoord + uint2(0, 1), uint2(InputWidth - 1, InputHeight - 1)), 0));
	float Depth3 = InputHiZTexture.Load(int3(min(SourceCoord + uint2(1, 1), uint2(InputWidth - 1, InputHeight - 1)), 0));

	float MaxDepth = max(max(Depth0, Depth1), max(Depth2, Depth3));

	OutputHiZTexture[DispatchThreadID.xy] = MaxDepth;
}
