#define EPSILON 1e-6

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
    float4 Color; // light color
    float3 Position; // world space position
    float Intensity;
    float AttenuationRadius; 
    float LightFalloffExponent; // exponent
    float2 Pad0;
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

cbuffer PerObject : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;
    row_major float4x4 WorldInverseTranspose;
    uint UUID;
    float3 Pad_uuid;
};

cbuffer Lighting : register(b10)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    float3 CameraPos; // world space camera position
    uint NumPointLights;
    uint NumSpotLights;
    float3 Pad1;
};

StructuredBuffer<FPointLightInfo> PointLights : register(t2);
StructuredBuffer<FSpotLightInfo> SpotLights : register(t3);

cbuffer PerMaterial : register(b11)
{
    float4 MaterialAmbient; // k_a (rgb)
    float4 MaterialDiffuse; // k_d (albedo, rgb)
    float4 MaterialSpecular; // k_s (specular, rgb)
    float4 MaterialEmissive; // emissive Color
   
    float SpecularShininess; // alpha
    uint HasNormalMap;
    float2 Pad2;
};
float3 CalculateAmbientLight(FAmbientLightInfo info)
{
    // ambient 계수는 밖에서 곱
    return info.Color.rgb * info.Intensity;
}
void CalculateDirectionalLight(FDirectionalLightInfo info, float3 N, float3 V, float shininess,
    out float3 outDiffuse,
    out float3 outSpecular
)
{
    float3 L = normalize(-info.Direction);
    float3 H = normalize(L + V);
    
    // diffuse term 
    float NdotL = saturate(dot(N, L));
    outDiffuse = info.Color.rgb * info.Intensity * NdotL; // diffuse 계수는 밖에서 곱
    
    // specular term
    float NdotH = saturate(dot(N, H));
    outSpecular = info.Color.rgb * info.Intensity * pow(NdotH, shininess); // specular 계수는 밖에서 곱
}
void CalculatePointLight(FPointLightInfo info, float3 worldPos, float3 N, float3 V, float shininess,
    out float3 outDiffuse,
    out float3 outSpecular
)
{
    float3 LDir = info.Position - worldPos;
    float distance = length(LDir);
    if(distance <= EPSILON || info.AttenuationRadius <= EPSILON || info.LightFalloffExponent <= EPSILON)
    {
        outDiffuse = float3(0.0f, 0.0f, 0.0f);
        outSpecular = float3(0.0f, 0.0f, 0.0f);
        return;
    }
    float3 L = normalize(LDir);
    float3 H = normalize(L + V);
    
    // attenuation
    float attenuation = pow(saturate(1 - distance / info.AttenuationRadius), info.LightFalloffExponent);
    
    // diffuse term (diffuse 계수는 밖에서 곱)
    float NdotL = saturate(dot(N, L));
    outDiffuse = info.Color.rgb * info.Intensity * NdotL * attenuation;
    
    // specular term (specular 계수는 밖에서 곱)
    float NdotH = saturate(dot(N, H));
    outSpecular = info.Color.rgb * info.Intensity * pow(NdotH, shininess) * attenuation;
    
}
void CalculateSpotLight(FSpotLightInfo info, float3 worldPos, float3 N, float3 V, float shininess,
    out float3 outDiffuse,
    out float3 outSpecular
)
{
    float3 LDir = info.Position - worldPos;
    float distance = length(LDir);
    if(distance <= EPSILON || info.AttenuationRadius <= EPSILON || info.LightFalloffExponent <= EPSILON)
    {
        outDiffuse = float3(0.0f, 0.0f, 0.0f);
        outSpecular = float3(0.0f, 0.0f, 0.0f);
        return;
    }
    float3 L = normalize(LDir);
    float3 H = normalize(L + V);
    
    float3 spotAxis = normalize(-info.Direction);
    float cosTheta = dot(L, spotAxis);
    
    float spot = 0.0f;
    
    spot = saturate((cosTheta - info.OuterConeAngle) / (info.InnerConeAngle - info.OuterConeAngle));
    
    float attenuation = pow(saturate(1 - distance / info.AttenuationRadius), info.LightFalloffExponent);
    
    // diffuse term (diffuse 계수는 밖에서)
    float NdotL = saturate(dot(N, L));
    outDiffuse = info.Color.rgb * info.Intensity * NdotL * attenuation * spot;
    
    // specular term (specular 계수는 밖에서)
    float NdotH = saturate(dot(N, H));
    outSpecular = info.Color.rgb * info.Intensity * pow(NdotH, shininess) * attenuation * spot;

}

float3 TransformPosToWorld(float3 pos)
{
    float4 wp = mul(float4(pos, 1.0), World);
    return wp.xyz;
}
float3 TransformNormalToWorld(float3 n)
{
    float3 w = mul(n, (float3x3)WorldInverseTranspose);
    return normalize(w);
}
struct VS_INPUT
{
    // 공통 속성
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float4 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    // 공통 속성
    float4 Position : SV_Position;
    float3 WorldPosition : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float4 Tangent : TANGENT0;
    uint UUID : UUID;
    
#if defined(LIGHTING_MODEL_GOURAUD)
    float3 Lit_Ambient : COLOR0; // already multiplied by k_a in VS for Gouraud
    float3 Lit_Diffuse : COLOR1; // already multiplied by k_d in VS for Gouraud
    float3 Lit_Specular : COLOR2; // already multiplied by k_s in VS for Gouraud
#endif 
};
struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};
VS_OUTPUT Uber_VS(VS_INPUT Input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    float3 worldPos = TransformPosToWorld(Input.Position);
    float3 worldN = TransformNormalToWorld(Input.Normal);
    output.WorldPosition = worldPos;
    output.WorldNormal = worldN;
    output.UV = Input.UV;
    output.Position = mul(float4(Input.Position, 1.0f), World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);
    output.Tangent = Input.Tangent;
    output.UUID = UUID;
    
#if defined(LIGHTING_MODEL_GOURAUD)
    // calculate in vertex
    float3 V = normalize(CameraPos - worldPos);
    
    float3 ambientRaw = CalculateAmbientLight(Ambient);
    
    float3 diffuseRaw = float3(0.0f, 0.0f, 0.0f);
    float3 specularRaw = float3(0.0f, 0.0f, 0.0f);
    
    // 임시 변수
    float3 diffuseTemp = float3(0.0f, 0.0f, 0.0f);
    float3 specularTemp = float3(0.0f, 0.0f, 0.0f);
    
    // Directional 계산
    CalculateDirectionalLight(Directional, worldN, V, SpecularShininess, diffuseTemp, specularTemp);
    diffuseRaw += diffuseTemp;
    specularRaw += specularTemp;
    
    for (uint i = 0; i < NumPointLights; ++i)
    {
        CalculatePointLight(PointLights[i], worldPos, worldN, V, SpecularShininess, diffuseTemp, specularTemp);
        diffuseRaw += diffuseTemp;
        specularRaw += specularTemp;
    }
    
    for (uint j= 0; j < NumSpotLights; ++j)
    {
        CalculateSpotLight(SpotLights[j], worldPos, worldN, V, SpecularShininess, diffuseTemp, specularTemp);
        diffuseRaw += diffuseTemp;
        specularRaw += specularTemp;
    }
    
    // material 계수들은 한번에 곱하기 (k_a, k_d, k_s)
    float3 ambientTerm = ambientRaw * MaterialAmbient.rgb;
    float3 diffuseTerm = diffuseRaw * MaterialDiffuse.rgb;
    float3 specularTerm = specularRaw * MaterialSpecular.rgb;
    
    output.Lit_Ambient = ambientTerm;
    output.Lit_Diffuse = diffuseTerm;
    output.Lit_Specular = specularTerm;
#endif
    output.UUID = UUID;
    return output;
}
Texture2D TextureColor : register(t0);
SamplerState Sampler : register(s0);

Texture2D NormalMapTex : register(t1);

PS_OUTPUT Uber_PS(VS_OUTPUT Input) : SV_Target
{
    PS_OUTPUT output;
    float4 FinalPixel = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 albedoTexture = TextureColor.Sample(Sampler, Input.UV).rgb;
    
    float3 N = 0.0f;
    float2 UV = Input.UV;
    
#if defined(HAS_NORMAL_MAP)
    // TBN과 NormalMap으로부터 월드 노말 구하기
    if (HasNormalMap == 1)
    {
        N = NormalMapTex.Sample(Sampler, UV).xyz;
    
        N = 2.0f * N - 1.0f;
        N = normalize(N);
        
        float3 Nw = normalize(Input.WorldNormal);
        float3 Tw = normalize(Input.Tangent.xyz);
        float h = Input.Tangent.w; // handedness (+1/-1)
        float3 Bw = normalize(cross(Nw, Tw) * h);
    
        float3x3 TBN = float3x3(Tw, Bw, Nw);
        N = normalize(mul(N, TBN));
    }
    else
    {
        N = Input.WorldNormal;
    }
    
#else
    N = Input.WorldNormal;
    
#endif
    
        float3 k_a = MaterialAmbient.rgb;
        float3 k_d = MaterialDiffuse.rgb;
        float3 k_s = MaterialSpecular.rgb;
        float3 k_e = MaterialEmissive.rgb;
        float shininess = SpecularShininess;
    
#if defined(LIGHTING_MODEL_GOURAUD)
    // VS에서 이미 재질 계수(k_a, k_d, k_s)가 모두 곱해짐
    float3 finalLighting = Input.Lit_Ambient + (Input.Lit_Diffuse * albedoTexture) + Input.Lit_Specular;
    FinalPixel.rgb = finalLighting + k_e;
    FinalPixel.a = 1.0f;
#elif defined(LIGHTING_MODEL_LAMBERT)
    float3 V = normalize(CameraPos - Input.WorldPosition);
    
    float3 ambientRaw = CalculateAmbientLight(Ambient);
    float3 diffuseRaw = float3(0.0f, 0.0f, 0.0f);
    
    float3 diffuseTemp, specularTemp;
    
    // Directional
    CalculateDirectionalLight(Directional, N, V, shininess, diffuseTemp, specularTemp);
    diffuseRaw += diffuseTemp;
    
    // Point
    for (uint i = 0; i < NumPointLights; ++i)
    {
        CalculatePointLight(PointLights[i], Input.WorldPosition, N, V, shininess, diffuseTemp, specularTemp);
        diffuseRaw += diffuseTemp;
    }
    // Spot
    for (uint j = 0; j < NumSpotLights; ++j)
    {
        CalculateSpotLight(SpotLights[j], Input.WorldPosition, N, V, shininess, diffuseTemp, specularTemp);
        diffuseRaw += diffuseTemp;
    }
    float3 ambientTerm = ambientRaw * k_a;
    float3 diffuseTerm = diffuseRaw * k_d * albedoTexture;
    
    float3 finalLighting = ambientTerm + diffuseTerm + k_e;
    FinalPixel = float4(finalLighting, 1.0f);
#elif defined(LIGHTING_MODEL_PHONG)
    float3 V = normalize(CameraPos - Input.WorldPosition);
    
    float3 ambientRaw = CalculateAmbientLight(Ambient);
    float3 diffuseRaw = float3(0.0f, 0.0f, 0.0f);
    float3 specularRaw = float3(0.0f, 0.0f, 0.0f);
    
    float3 diffuseTemp, specularTemp;
    
    // Directional
    CalculateDirectionalLight(Directional, N, V, shininess, diffuseTemp, specularTemp);
    diffuseRaw += diffuseTemp;
    specularRaw += specularTemp;
    
    // Point
    for (uint i = 0; i < NumPointLights; ++i)
    {
        CalculatePointLight(PointLights[i], Input.WorldPosition, N, V, shininess, diffuseTemp, specularTemp);
        diffuseRaw += diffuseTemp;
        specularRaw += specularTemp;
    }
    // Spot
    for (uint j = 0; j < NumSpotLights; ++j)
    {
        CalculateSpotLight(SpotLights[j], Input.WorldPosition, N, V, shininess, diffuseTemp, specularTemp);
        diffuseRaw += diffuseTemp;
        specularRaw += specularTemp;
    }
    float3 ambientTerm = ambientRaw * k_a;
    float3 diffuseTerm = diffuseRaw * k_d * albedoTexture;
    float3 specularTerm = specularRaw * k_s;
    
    float3 finalLighting = ambientTerm + diffuseTerm + specularTerm + k_e;
    FinalPixel = float4(finalLighting, 1.0f);
#endif
        output.Color = FinalPixel;
        output.UUID = Input.UUID;
        return output;
    }
