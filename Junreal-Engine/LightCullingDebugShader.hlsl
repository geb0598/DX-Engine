Texture2D FrameColor : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D Heatmap : register(t2);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

struct PS_Input
{
    float4 PositionCS : SV_Position;
    float2 UV : TEXCOORD0;
};

PS_Input mainVS(uint Input : SV_VertexID)
{
    PS_Input Output;

    float2 UVMap[] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
    };

    Output.UV = UVMap[Input];
    Output.PositionCS = float4(Output.UV.x * 2.0f - 1.0f, 1.0f - (Output.UV.y * 2.0f), 0.0f, 1.0f);
    return Output;
}

float4 mainPS(PS_Input Input) : SV_Target
{
    uint Width, Height;
    DepthTexture.GetDimensions(Width, Height);

    /** @note Do not use Input.UV. Use UVPosition calculated from screen size instead. */
    float2 UVPosition = Input.PositionCS.xy / float2(Width,Height); 

    float4 SampledFrameColor = FrameColor.Sample(LinearSampler, UVPosition);
    float4 SampledHeatmapColor = Heatmap.Sample(LinearSampler, UVPosition);

    float4 FinalColor = SampledFrameColor + SampledHeatmapColor;

    return FinalColor;
}