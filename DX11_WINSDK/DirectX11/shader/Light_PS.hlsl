#include "Light.hlsli"

float4 MainPS(VertexOut pIn) : SV_Target
{
    pIn.NormalW = normalize(pIn.NormalW);
	
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);
	
    float4 ambient, diffuse, spec;
    float4 A, D, S;
    ambient = diffuse = spec = A = D = S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    ComputeDirectionalLight(g_Material, g_DirLight, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    ComputePointLight(g_Material, g_PointLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    ComputeSpotLight(g_Material, g_SpotLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    float4 litColor = pIn.Color * (ambient + diffuse) + spec;
    
    litColor.a = g_Material.Diffuse.a * pIn.Color.a;
	
    return litColor;
}