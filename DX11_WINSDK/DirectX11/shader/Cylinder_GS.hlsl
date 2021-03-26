#include "Basic.hlsli"

[maxvertexcount(6)]
void GS( line VertexPosHWNormalColor input[2], inout TriangleStream<VertexPosHWNormalColor> output)
{
    float3 upDir = normalize(cross(input[0].NormalW, (input[1].PosW - input[0].PosW)));
    VertexPosHWNormalColor v2, v3;
    
    matrix viewProj = mul(g_View, g_Proj);
    
    v2.PosW = input[1].PosW + upDir * g_CylinderHeight;
    v2.PosH = mul(float4(v2.PosW, 1.0f), viewProj);
    v2.NormalW = input[1].NormalW;
    v2.Color = input[1].Color;
    
    v3.PosW = input[0].PosW + upDir * g_CylinderHeight;
    v3.PosH = mul(float4(v3.PosW, 1.0f), viewProj);
    v3.NormalW = input[0].NormalW;
    v3.Color = input[0].Color;
    
    output.Append(input[0]);
    output.Append(input[1]);
    output.Append(v2);
    output.RestartStrip();
    
    output.Append(v2);
    output.Append(v3);
    output.Append(input[0]);

}