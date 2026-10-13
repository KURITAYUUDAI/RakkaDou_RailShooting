#include "Environment.hlsli"
#include "../tool/Dissolve.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ===== [テキストより独自で変換したポイント] =======
// float32_t4 → float4
//                                         by ChatGPT
// ==================================================

TextureCube<float4> gEnvironmentTexture : register(t1);

struct Material
{
    float4 color;
    
    int enableLighting;
    float shiniess;
    float environmentCoefficient;
    float alphaReference;
    
    row_major float4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct DirectionalLight
{
    float4 color;     //!< ライトの色
    float3 direction; //!< ライトの方向
    float intensity;  //!< 輝度
};
struct PointLight
{
    float4 color;    //!< ライトの色
    float3 position; //!< ライトの位置
    float intensity; //!< 輝度
    float radius;    //!< ライトの届く最大距離
    float decay;     //!< 減衰率
    float2 padding;  //!< パディング
};
struct SpotLight
{
    float4 color;     //!< ライトの色
    float3 position;  //!< ライトの位置
    float intensity;  //!< 輝度
    float3 direction; //!< スポットライトの方向
    float distance;   //!< ライトの届く最大距離
    float decay;      //!< 減衰率
    float cosAngle;   //!< スポットライトの角度
    float cosFalloff; //!< スポットライトのFalloffが始まる角度
    float padding;   //!< パディング
};
struct ConstBufferLights
{
    DirectionalLight directionalLight;
    PointLight pointLights[16];
    int numPointLights; // 現在有効なポイントライト数
    float3 padding;
    SpotLight spotLights[16]; // 最大スポットライト数
    int numSpotLights; // 現在有効なスポットライト数
    float3 padding2;
};
ConstantBuffer<ConstBufferLights> gLights : register(b1);

struct Camera
{
    float3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float> gMaskTexture : register(t2);
ConstantBuffer<DissolveParams> gDissolve : register(b3);

struct DistanceFog
{
    float4 color;
    float startDistance;
    float endDistance;
    int enabled;
    float padding;
};
ConstantBuffer<DistanceFog> gDistanceFog : register(b4);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};



PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    float alpha = gMaterial.color.a * textureColor.a;

    if (alpha < gMaterial.alphaReference)
    {
        discard;
    }
    
    PixelShaderOutput output;
    
    
    if(gMaterial.enableLighting != 0)   // Lightningする場合
    {
        // 法線ベクトル
        float3 inputNormal = normalize(input.normal);
        // 平行光源の方向ベクトル
        float3 directionalLightDirection = normalize(-gLights.directionalLight.direction);
      
        
        //// Lambert
        //float cos = saturate(dot(normalize(input.normal), -gLights.directionalLight.direction));
        
        // 内積
        float NdotL = saturate(dot(inputNormal, directionalLightDirection)); // Normal dot LightDirection
        // Half Lambert
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
       
        // 視線ベクトル
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        // 反射光ベクトル
        float3 reflectLight = reflect(-directionalLightDirection, inputNormal);
        // 視線と反射光の角度
        float RdotE = dot(reflectLight, toEye);
        // 鏡面反射の強さを計算
        float specularPow = pow(saturate(RdotE), gMaterial.shiniess);
        // 影になる部分の鏡面反射を消す
        specularPow *= step(0.0001f, NdotL);
        
        // 拡散反射
        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gLights.directionalLight.color.rgb 
            * cos * gLights.directionalLight.intensity;
        
        // 鏡面反射
        float3 specular = gLights.directionalLight.color.rgb * gLights.directionalLight.intensity 
            * specularPow * float3(1.0f, 1.0f, 1.0f);
        
        float3 pointLightDirection[16];
        float3 pointDiffuse[16];
        float3 pointSpecular[16];
        
        for (int i = 0; i < gLights.numPointLights; i++)
        {
             // 点光源の入射光ベクトル
            pointLightDirection[i] = normalize(gLights.pointLights[i].position - input.worldPosition);
            
            //float distance = length(gLights.pointLights[i].position - input.worldPosition);
            //float factor = 1.0f / (distance * distance);
            
            float distance = length(gLights.pointLights[i].position - input.worldPosition);
            float factor = pow(saturate(-distance / gLights.pointLights[i].radius + 1.0), gLights.pointLights[i].decay);
            
            // 点光源の拡散反射
            pointDiffuse[i] = gMaterial.color.rgb * textureColor.rgb * gLights.pointLights[i].color.rgb 
                * saturate(dot(inputNormal, pointLightDirection[i])) * gLights.pointLights[i].intensity * factor;

            // 点光源の鏡面反射
            float3 pointReflectLight = reflect(-pointLightDirection[i], inputNormal);
            float RdotE_Point = dot(pointReflectLight, toEye);
            float specularPow_Point = pow(saturate(RdotE_Point), gMaterial.shiniess);
            specularPow_Point *= step(0.0001f, saturate(dot(inputNormal, pointLightDirection[i])));
            
            pointSpecular[i] = gLights.pointLights[i].color.rgb * gLights.pointLights[i].intensity * factor
                * specularPow_Point * float3(1.0f, 1.0f, 1.0f);
            
            diffuse += pointDiffuse[i];
            specular += pointSpecular[i];
        }
        
        float3 spotLightDirectionOnSurface[16];
        float3 spotDiffuse[16];
        float3 spotSpecular[16];
        for (int j = 0; j < gLights.numSpotLights; j++)
        {
            spotLightDirectionOnSurface[j] = normalize(gLights.spotLights[j].position - input.worldPosition);
            
            float distance = length(gLights.spotLights[j].position - input.worldPosition);
            float attenuationFactor = pow(saturate(-distance / gLights.spotLights[j].distance + 1.0), gLights.spotLights[j].decay);
            
            float cosAngle = dot(-spotLightDirectionOnSurface[j], normalize(gLights.spotLights[j].direction));
            float falloffFactor = saturate((cosAngle - gLights.spotLights[j].cosAngle) / (gLights.spotLights[j].cosFalloff - gLights.spotLights[j].cosAngle));
            
            // スポットライトの拡散反射
            spotDiffuse[j] = gMaterial.color.rgb * textureColor.rgb * gLights.spotLights[j].color.rgb 
                * saturate(dot(inputNormal, spotLightDirectionOnSurface[j])) * gLights.spotLights[j].intensity * attenuationFactor * falloffFactor;

            // スポットライトの鏡面反射
            float3 spotReflectLight = reflect(-spotLightDirectionOnSurface[j], inputNormal);
            float RdotE_Spot = dot(spotReflectLight, toEye);
            float specularPow_Spot = pow(saturate(RdotE_Spot), gMaterial.shiniess);
            specularPow_Spot *= step(0.0001f, saturate(dot(inputNormal, spotLightDirectionOnSurface[j])));
            
            spotSpecular[j] = gLights.spotLights[j].color.rgb * gLights.spotLights[j].intensity * attenuationFactor * falloffFactor
                * specularPow_Spot * float3(1.0f, 1.0f, 1.0f);
            
            diffuse += spotDiffuse[j];
            specular += spotSpecular[j];
        }
        
        // 拡散反射＋鏡面反射
        output.color.rgb = diffuse + specular;
        // アルファは今まで通り
        output.color.a = gMaterial.color.a * textureColor.a;
        
        float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float3 reflectedVector = reflect(cameraToPosition, normalize(inputNormal));
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        
        output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;

      
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    
    float mask = gMaskTexture.Sample(gSampler, input.texcoord);
    ApplyDissolve(mask, gDissolve.threshold, gDissolve.edgeColor, output.color);

    if (gDistanceFog.enabled != 0)
    {
        float distanceFromCamera = length(gCamera.worldPosition - input.worldPosition);
        float fogRange = max(gDistanceFog.endDistance - gDistanceFog.startDistance, 0.001f);
        float fogFactor = saturate(
            (distanceFromCamera - gDistanceFog.startDistance) / fogRange);
        fogFactor = smoothstep(0.0f, 1.0f, fogFactor);

        output.color.rgb = lerp(output.color.rgb, gDistanceFog.color.rgb, fogFactor);
    }
    
    //float3 N = normalize(input.normal);
    
    //output.color = float4(N * 0.5f + 0.5f, 1.0f);

    return output;
}
