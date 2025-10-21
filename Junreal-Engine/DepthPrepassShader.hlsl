// DepthPrepassShader.hlsl

cbuffer ModelBuffer : register(b0)
{
    row_major matrix World;
};

cbuffer ViewProjBuffer : register(b1)
{
    row_major matrix View;
    row_major matrix Projection;
};

struct VS_INPUT
{
    float3 Position : POSITION;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
};
    
VS_OUTPUT mainVS(VS_INPUT Input)
{
    VS_OUTPUT Output;
    Output.Position = mul(float4(Input.Position, 1.0f), World);
    Output.Position = mul(mul(Output.Position, View), Projection);
    return Output;
}

void PS()
{
    // nothing-to-do.
}