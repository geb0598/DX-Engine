cbuffer WorldMatrixBuffer : register(b0)
{
	row_major float4x4 World;
};

cbuffer ViewProjBuffer : register(b1)
{
	row_major float4x4 View;
	row_major float4x4 Projection;
};

struct VS_INPUT
{
	float4 Position : POSITION;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
	VS_OUTPUT Output;

	float4 WorldPos = mul(input.Position, World);
	float4 ViewPos = mul(WorldPos, View);
	Output.Position = mul(ViewPos, Projection);

	return Output;
}
