cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
    float gTime;
    float4 gPulseColor;
};

float4 PS(float4 PosH : SV_POSITION, float4 Color : COLOR) : SV_Target
{
    const float pi = 3.14159;
    float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;
    float4 c = lerp(Color, gPulseColor, s);
    //clip(Color.r - 0.5f);
    return c;
}