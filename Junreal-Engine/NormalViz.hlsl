cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    uint UUID;
    float3 Padding;
    row_major float4x4 NormalMatrix; // inverse-transpose(World)
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    float3 CameraWorldPos;
    float _pad_cam;
}

// Toggle for using TBN normal mapping
cbuffer FNormalVizCB : register(b10)
{
    uint bUseTBN;
    uint _pad0;
    uint _pad1;
    uint _pad2;
}

Texture2D g_NormalMapTex : register(t0);
SamplerState g_Sample : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL0;
    float4 color    : COLOR;
    float2 texCoord : TEXCOORD0;
    float4 tangent  : TANGENT; // xyz=tangent, w=handedness
};

struct PS_INPUT
{
    float4 position     : SV_POSITION;
    float3 worldPos     : TEXCOORD0;
    float3 worldNormal  : TEXCOORD1;
    float2 texCoord     : TEXCOORD2;
    float4 tangent      : TANGENT;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT o;

    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);
    o.worldPos = worldPos.xyz;

    // Transform normal and tangent to world
    float3 Nw = normalize(mul(input.normal, (float3x3)NormalMatrix));
    float3 Tw = normalize(mul(input.tangent.xyz, (float3x3)NormalMatrix));

    o.worldNormal = Nw;
    o.tangent = float4(Tw, input.tangent.w);
    o.texCoord = input.texCoord;

    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    o.position = mul(float4(input.position, 1.0f), MVP);
    return o;
}

float3 NormalToColor(float3 n)
{
    return 0.5f * (normalize(n) + 1.0f);
}

float3 ComputeTBNNormal(float3 Nw, float4 tangent, float2 uv)
{
    // Sample tangent-space normal map [0,1] -> [-1,1]
    float3 nTS = g_NormalMapTex.Sample(g_Sample, uv).xyz * 2.0f - 1.0f;

    float3 T = normalize(tangent.xyz);
    float3 B = normalize(cross(Nw, T)) * tangent.w; // handedness in w
    float3x3 TBN = float3x3(T, B, Nw);
    return normalize(mul(nTS, TBN));
}

float4 mainPS(PS_INPUT input) : SV_Target0
{
    float3 N = input.worldNormal;
    if (bUseTBN != 0)
    {
        N = ComputeTBNNormal(N, input.tangent, input.texCoord);
    }

    float3 rgb = NormalToColor(N);
    return float4(rgb, 1.0f);
}

