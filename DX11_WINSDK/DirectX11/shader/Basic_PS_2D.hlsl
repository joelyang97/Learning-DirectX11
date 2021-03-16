#include "Basic.hlsli"

float4 PS_2D(VertexPosHTex pIn) : SV_TARGET
{
    return gTex.Sample(gSamLinear, pIn.Tex);
}