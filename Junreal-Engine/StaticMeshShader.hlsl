cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    uint UUID;
    float3 Padding;
    row_major float4x4 NormalMatrix;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    float3 CameraWorldPos; // 월드 기준 카메라 위치
    float _pad_cam; // 16바이트 정렬
}

cbuffer HighLightBuffer : register(b2)
{
    int Picked;
    float3 Color;
    int X;
    int Y;
    int Z;
    int GIzmo;
    int enable;
}

Texture2D g_DiffuseTexColor : register(t0);
SamplerState g_Sample : register(s0);

Texture2D g_NormalMapTex : register(t1);

struct FMaterial
{
    float3 DiffuseColor; // Kd
    float OpticalDensity; // Ni
    float3 AmbientColor; // Ka
    float Transparency; // Tr/d
    float3 SpecularColor; // Ks
    float SpecularExponent; // Ns
    float3 EmissiveColor; // Ke
    uint IlluminationModel; // illum
    float3 TransmissionFilter; // Tf
    float dummy;
};

cbuffer ColorBuffer : register(b3)
{
    float4 LerpColor;
}

cbuffer PixelConstData : register(b4)
{
    FMaterial Material;
    bool HasMaterial;
    bool HasTexture;
}

cbuffer PSScrollCB : register(b5)
{
    float2 UVScrollSpeed;
    float UVScrollTime;
    float _pad_scrollcb;
}

#define MAX_PointLight 100

// C++ 구조체와 동일한 레이아웃
struct FPointLightData
{
    float4 Position; // xyz=위치(월드), w=반경
    float4 Color; // rgb=색상, a=Intensity
    float FallOff; // 감쇠 지수
    float3 _pad; // 패딩
};

cbuffer PointLightBuffer : register(b9)
{
    int PointLightCount;
    float3 _pad;
    FPointLightData PointLights[MAX_PointLight];
}

struct LightAccum
{
    float3 diffuse;
    float3 specular;
};

// ------------------------------------------------------------------
// 안정화된 감쇠 + 방향(표면→광원) 버전의 simple 누적
// ------------------------------------------------------------------
float3 ComputePointLights(float3 worldPos)
{
    float3 total = 0;
    [loop]
    for (int i = 0; i < PointLightCount; ++i)
    {
        float3 Lvec = PointLights[i].Position.xyz - worldPos;
        float dist = length(Lvec);
        float range = max(PointLights[i].Position.w, 1e-3);
        float fall = max(PointLights[i].FallOff, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb * PointLights[i].Color.a;
        total += Li * atten;
    }
    return total;
}

// ------------------------------------------------------------------
// Lambert + Blinn-Phong (안정/일관성)
// ------------------------------------------------------------------
LightAccum ComputePointLights_LambertPhong(float3 worldPos, float3 worldNormal, float shininess)
{
    LightAccum acc = (LightAccum) 0;

    float3 N = normalize(worldNormal);
    float3 V = normalize(CameraWorldPos - worldPos); // 픽셀 기준 뷰 벡터(월드)

    float exp = clamp(shininess, 1.0, 128.0); // 폭발 방지

    [loop]
    for (int i = 0; i < PointLightCount; ++i)
    {
        float3 Lvec = PointLights[i].Position.xyz - worldPos; // 표면→광원
        float dist = length(Lvec);
        float3 L = (dist > 1e-5) ? (Lvec / dist) : float3(0, 0, 1);

        float range = max(PointLights[i].Position.w, 1e-3);
        float fall = max(PointLights[i].FallOff, 0.001);
        float t = saturate(dist / range);
        float atten = pow(saturate(1.0 - t), fall);

        float3 Li = PointLights[i].Color.rgb * PointLights[i].Color.a;

        // Diffuse
        float NdotL = saturate(dot(N, L));
        float3 diffuse = Li * NdotL * atten;

        // Specular (Blinn-Phong)
        float3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        float3 specular = Li * pow(NdotH, exp) * atten;

        acc.diffuse += diffuse;
        acc.specular += specular;
    }

    return acc;
}

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD2;
    float4 tangent : TANGENT;
    uint UUID : UUID;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT o;
    
    float time = UVScrollTime;

    // 기본 위치
    float3 displacedPos = input.position;

    // 🔥 enable == 1 일 때만 일렁임 적용
    if (Picked == 1 && enable == 1)
    {
    // 🔹 노멀 기반 일렁임 (진폭 ↑, 주파수 ↓)
        float wave = sin(time * 3.5 + input.position.x * 3.0 + input.position.y * 3.0) * 0.08;

    // 🔹 중심 기반 라디얼 펄스 (진폭 ↑, 속도 ↑)
        float dist = length(input.position.xy);
        wave += sin(dist * 8.0 - time * 6.0) * 0.06;

    // 🔥 약간의 난수성 섞기 (불규칙한 표면)
        float noise = sin((input.position.x + input.position.y + input.position.z) * 20.0 + time * 10.0) * 0.02;
        wave += noise;

    // 최종 displacement 적용
        displacedPos = input.position + input.normal * wave;
    }
    // 월드 변환 (row_major 기준: mul(v, M))
    float4 worldPos = mul(float4(displacedPos, 1.0f), WorldMatrix);
    o.worldPosition = worldPos.xyz;

    // 노멀: inverse-transpose(World)
    o.worldNormal = normalize(mul(input.normal, (float3x3) NormalMatrix));

    // MVP
    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    o.position = mul(float4(displacedPos, 1.0f), MVP);

    // Gizmo 색상 처리
    float4 c = input.color;
    if (GIzmo == 1)
    {
        if (Y == 1)
            c = float4(1.0, 1.0, 0.0, c.a);
        else
        {
            if (X == 1)
                c = float4(1.0, 0.0, 0.0, c.a);
            else if (X == 2)
                c = float4(0.0, 1.0, 0.0, c.a);
            else if (X == 3)
                c = float4(0.0, 0.0, 1.0, c.a);
        }
    }

    o.color = c;
    o.texCoord = input.texCoord;
    
    float3x3 World3x3 = (float3x3) WorldMatrix;
    
    float3 tWS = normalize(mul(input.tangent.xyz, (float3x3) NormalMatrix));
    o.tangent = float4(tWS, input.tangent.w);

    o.UUID = UUID;
    
    return o;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT Result;

    float3 base = input.color.rgb;
    base = lerp(base, LerpColor.rgb, LerpColor.a) * (1.0f - (HasMaterial ? 1.0f : 0.0f));

    if (HasMaterial && HasTexture)
    {
        float2 uv = input.texCoord + UVScrollSpeed * UVScrollTime;
        base = g_DiffuseTexColor.Sample(g_Sample, uv).rgb;
    }

    if (Picked == 1)
    {
        base = lerp(base, float3(1.0, 1.0, 0.0), 0.5); // 하이라이트
        if (enable == 1)
        {
        // 🔥 태양 일렁임 효과: 복합 노이즈 UV 왜곡
            float time = UVScrollTime * 2.5; // 시간 속도 조절

        // 기본 UV
            float2 uv = input.texCoord;

        // 중심에서의 거리 기반 왜곡 (라디얼)
            float2 center = float2(0.5, 0.5);
            float2 toCenter = uv - center;
            float dist = length(toCenter);
            
        // 다중 sine 기반 노이즈 왜곡
            float wave1 = sin(time + dist * 25.0) * 0.02;
            float wave2 = sin(time * 1.7 + (uv.x + uv.y) * 40.0) * 0.015;
            float wave3 = sin(time * 2.3 + uv.x * 60.0) * 0.01;

            float2 distortion = float2(wave1 + wave2, wave2 + wave3);

            float2 animatedUV = uv + distortion;

        // 🔥 텍스처 색상
            float3 texColor = g_DiffuseTexColor.Sample(g_Sample, animatedUV).rgb;

        // 중심 발광 강화
            float glow = saturate(1.0 - dist * 2.0);
            texColor += glow * float3(1.2, 0.4, 0.1);

        // 최종 결과
            Result.Color = float4(saturate(texColor), 1.0);
            Result.UUID = input.UUID;
            return Result;
        }
    }

    // 조명 계산 (shininess는 Material.SpecularExponent를 쓰는 게 일반적)
    //float3 N = normalize(input.worldNormal);
    float2 uv = input.texCoord + UVScrollSpeed * UVScrollTime;
    float3 N = g_NormalMapTex.Sample(g_Sample, uv).xyz;
    
    N = 2.0f * N - 1.0f;
    N = normalize(N);
    
    float3 Nw = normalize(input.worldNormal);
    float3 Tw = normalize(input.tangent.xyz);
    float h = input.tangent.w; // handedness (+1/-1)
    float3 Bw = normalize(cross(Nw, Tw) * h);
    
    float3x3 TBN = float3x3(Tw, Bw, Nw);
    N = normalize(mul(N, TBN));

    
    float shininess = (HasMaterial ? Material.SpecularExponent : 32.0); // 기본값 32
    LightAccum la = ComputePointLights_LambertPhong(input.worldPosition, N, shininess);
    float3 light = ComputePointLights(input.worldPosition);
    // Ambient + Diffuse + Specular
    float3 ambient = 0.25 * base;
    if (HasMaterial)
        ambient += 0.25 * Material.AmbientColor;

    float3 diffuseLit = base * la.diffuse;
    float3 specularLit = la.specular;
    if (HasMaterial)
        specularLit *= saturate(Material.SpecularColor);

    float3 finalLit = ambient + diffuseLit + specularLit;
    finalLit = saturate(finalLit); // 과포화 방지
    
    Result.Color = float4(finalLit, 1.0);
    Result.UUID = input.UUID;
    return Result;
}
