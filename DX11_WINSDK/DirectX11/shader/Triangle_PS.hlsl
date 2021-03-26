#include "Basic.hlsli"

float4 PS(VertexPosHColor pIn) : SV_TARGET
{
	return pIn.Color;
}