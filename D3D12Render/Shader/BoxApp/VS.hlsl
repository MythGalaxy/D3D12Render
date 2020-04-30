cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
    float gTime;
    float4 gPulseColor;
};

//float gTime : register(b1);

void VS(float3 iPosL : POSITION, float4 iColor : COLOR,
    out float4 oPosH : SV_POSITION, out float4 oColor : COLOR)
{
    //iPosL.xy += 0.5f * sin(iPosL.x) * sin(3.0f * gTime);
    //iPosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);
    oPosH = mul(float4(iPosL, 1.f), gWorldViewProj);
    
    oColor = iColor;
}