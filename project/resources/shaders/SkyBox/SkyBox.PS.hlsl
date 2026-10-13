#include "SkyBox.hlsli"

TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ===== [テキストより独自で変換したポイント] =======
// float32_t4 → float4
//                                         by ChatGPT
// ==================================================

struct Material
{
    float4 color;
    int enableLighting;
    row_major float4x4 uvTransform;
    float shiniess;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct DistanceFog
{
    float4 color;
    float startDistance;
    float endDistance;
    int enabled;
    float padding;
};
ConstantBuffer<DistanceFog> gDistanceFog : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    //float alpha = gMaterial.color.a * textureColor.a;

    //if (alpha < gMaterial.alphaReference)
    //{
    //    discard;
    //}
    
    PixelShaderOutput output;
    
    output.color = textureColor * gMaterial.color;

    // Skyboxは無限遠として扱い、フォグ有効時は洞窟奥の色で完全に塞ぐ。
    if (gDistanceFog.enabled != 0)
    {
        output.color.rgb = gDistanceFog.color.rgb;
    }

    return output;
}
