#include "Basic.hlsli"

float4 PS( VertexPosHWNormalColor pIn ) : SV_Target
{
    pIn.NormalW = normalize(pIn.NormalW);
    
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);
    
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    ComputeDirectionalLight(g_Material, g_DirLight[0], pIn.NormalW, toEyeW, ambient, diffuse, spec);
    
    return pIn.Color * (ambient + diffuse) + spec;
   
}