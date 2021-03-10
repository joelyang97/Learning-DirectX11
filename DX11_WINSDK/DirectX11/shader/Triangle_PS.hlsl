#include "Triangle.hlsli"

float4 PS(VertexIn pIn) : SV_TARGET
{
	return pIn.color;
}