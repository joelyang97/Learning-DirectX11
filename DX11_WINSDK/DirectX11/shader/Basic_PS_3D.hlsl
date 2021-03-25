#include "Basic.hlsli"

float4 PS_3D(VertexPosHWNormalTex pIn) : SV_TARGET
{
    float4 texColor = gTex.Sample(gSamLinear, pIn.Tex);
    clip(texColor.a - 0.1f);
    
    pIn.NormalW = normalize(pIn.NormalW);
	
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);
	
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int i;
    
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        DirectionalLight dirLight = g_DirLight[i];
        [flatten]
        if (g_IsReflection)
        {
            dirLight.Direction = mul(dirLight.Direction, (float3x3) (g_Reflection));

        }
        ComputeDirectionalLight(g_Material, dirLight, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    
    PointLight pointLight;
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        pointLight = g_PointLight[i];
        [flatten]
        if (g_IsReflection)
        {
            pointLight.Position = (float3) mul(float4(pointLight.Position, 1.0f), g_Reflection);
        }
        ComputePointLight(g_Material, pointLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    
    SpotLight spotLight;
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        spotLight = g_SpotLight[i];
        [flatten]
        if (g_IsReflection)
        {
            spotLight.Position = (float3) mul(float4(spotLight.Position, 1.0f), g_Reflection);
            spotLight.Direction = mul(spotLight.Direction, (float3x3) g_Reflection);
        }
        ComputeSpotLight(g_Material, spotLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
	
    float4 litColor = texColor * (ambient + diffuse) + spec;
    litColor.a = texColor.a * g_Material.Diffuse.a;
    
    return litColor;
}