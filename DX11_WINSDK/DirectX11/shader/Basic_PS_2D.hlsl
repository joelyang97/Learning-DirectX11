#include "Basic.hlsli"

float4 PS_2D(VertexPosHTex pIn) : SV_TARGET
{

    float4 color = gTex.Sample(gSamLinear, pIn.Tex);
    clip(color.a - 0.1f);
    return color;
}