struct PS_INPUT
{
    float4 position : SV_POSITION; // Position from vertex shader
    float4 color : COLOR;          // Color from vertex shader
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
