#define NUM_POINT_LIGHT 4
#define NUM_SPOT_LIGHT 4
#define LIGHTING_MODEL_GOURAUD 10
#define LIGHTING_MODEL_LAMBERT 11
#define LIGHTING_MODEL_PHONG 12
#define HAS_NORMAL_MAP 1
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
    float3 CameraPos; // world space camera position
    float Pad0;
};

cbuffer Lighting : register(b1)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    FPointLightInfo PointLights[NUM_POINT_LIGHT];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT];

};
cbuffer PerMaterial : register(b2)
{
    float4 MaterialAmbient; // k_a (rgb)
    float4 MaterialDiffuse; // k_d (albedo, rgb)
    float4 MaterialSpecular; // k_s (specular, rgb)
    float4 MaterialEmissive; // emissive Color
    float SpecularShininess; // alpha
    float3 Pad1;
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
    if(distance <= 1e-5)
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
    if(distance <= 1e-5)
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
    if (cosTheta >= info.InnerConeAngle)
        spot = 1.0f;
    else if(cosTheta <= info.OuterConeAngle)
        spot = 0.0f;
    else
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
    float3 w = mul(float4(n, 0.0), World).xyz;
    return normalize(w);
}
struct VS_INPUT
{
    // 공통 속성
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
};

struct VS_OUTPUT
{
    // 공통 속성
    float4 Position : SV_Position;
    float3 WorldPosition : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    
#if defined(LIGHTING_MODEL_GOURAUD)
    float3 Lit_Ambient : COLOR0; // already multiplied by k_a in VS for Gouraud
    float3 Lit_Diffuse : COLOR1; // already multiplied by k_d in VS for Gouraud
    float3 Lit_Specular : COLOR2; // already multiplied by k_s in VS for Gouraud
#endif 
};

VS_OUTPUT Uber_VS(VS_INPUT Input)
{
//    VS_OUTPUT output = (VS_OUTPUT) 0;
    
//    float4 worldPos = mul(float4(Input.Position, 1.0f), World);
//    output.Position = mul(worldPos, View);
//    output.Position = mul(output.Position, Projection);
//    output.UV = Input.UV;
    
//#if (VIEW_MODE == LIGHTING_MODEL_GOURAUD)
//    float3 worldNormal = normalize(mul(Input.normal, (float3x3)World);
//    float3 totalLight;
//    totalLight += Directional.Color.rgb * saturate(dot(worldNormal, -normalize(Directional.Direction)));
    
//#elif (VIEW_MODE == LIGHTING_MODEL_LAMBERT) || (VIEW_MODE == LIGHTING_MODEL_PHONG)
//    output.WorldPosition = worldPos.xyz;
//    output.WorldNormal = normalize(mul(Input.Normal, (float3x3)World));
//#endif
    VS_OUTPUT output = (VS_OUTPUT) 0;
    float3 worldPos = TransformPosToWorld(Input.Position);
    float3 worldN = TransformNormalToWorld(Input.Normal);
    output.WorldPosition = worldPos;
    output.WorldNormal = worldN;
    output.UV = Input.UV;
    output.Position = mul(float4(Input.Position, 1.0f), World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);
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
    
    // material 계수들은 한번에 곱하기 (k_a, k_d, k_s)
    float3 ambientTerm = ambientRaw * MaterialAmbient.rgb;
    float3 diffuseTerm = diffuseRaw * MaterialDiffuse.rgb;
    float3 specularTerm = specularRaw * MaterialSpecular.rgb;
    
    output.Lit_Ambient = ambientTerm;
    output.Lit_Diffuse = diffuseTerm;
    output.Lit_Specular = specularTerm;
#endif
    return output;
}
Texture2D TextureColor : register(t0);
SamplerState Sampler : register(s0);

float4 Uber_PS(VS_OUTPUT Input) : SV_Target
{
    float4 finalPixel = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float3 albedoTexture = TextureColor.Sample(Sampler, Input.UV).rgb;
    
    float3 k_a = MaterialAmbient.rgb;
    float3 k_d = MaterialDiffuse.rgb;
    float3 k_s = MaterialSpecular.rgb;
    float3 k_e = MaterialEmissive.rgb;
    float shininess = SpecularShininess;
    
#if defined(LIGHTING_MODEL_GOURAUD)
    // material 계수는 이미 Vertex Shader에서 곱함
    float3 final = Input.Lit_Ambient + Input.Lit_Diffuse * albedoTexture + Input.Lit_Specular + k_e;
    finalPixel = float4(final, 1.0f);

#endif
    return finalPixel;
}