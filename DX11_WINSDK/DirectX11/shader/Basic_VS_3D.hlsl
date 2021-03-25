#include "Basic.hlsli"

VertexPosHWNormalTex VS_3D( VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    matrix viewProj = mul(g_View, g_Proj);
    float4 posW = mul(float4(vIn.PosL, 1.0f), g_World);
    float3 normalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    
    [flatten]
    if (g_IsReflection)
    {
        posW = mul(posW, g_Reflection);
        normalW = mul(normalW, (float3x3) g_Reflection);

    }
    
    [flatten]
    if (g_IsShadow)
    {
        posW = (g_IsReflection ? mul(posW, g_RefShadow) : mul(posW, g_Shadow));

    }
    
    vOut.PosH = mul(posW, viewProj);
    vOut.PosW = posW.xyz;
    vOut.NormalW = normalW;
    vOut.Tex = vIn.Tex;
    return vOut;
}