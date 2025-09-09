cbuffer constants
{
    int bIsSelected;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(PS_INPUT Input) : SV_TARGET
{
    // float Brightness = (bIsSelected == 0) ? 1.0f : 0.1f;
    float Brightness = (bIsSelected + 0.5f);

    return Input.Color * Brightness;
}
