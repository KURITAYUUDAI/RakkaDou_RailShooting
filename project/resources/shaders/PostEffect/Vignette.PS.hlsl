#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float threshold;
    float softness;
    float intensity;
    float padding;
    float4 darkColor;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};



PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 source = gTexture.Sample(gSampler, input.texcoord);
    float2 centered = input.texcoord - float2(0.5f, 0.5f);
    float distanceFromCenter = length(centered);
    float edgeMask = smoothstep(
        gMaterial.threshold,
        gMaterial.threshold + max(gMaterial.softness, 0.0001f),
        distanceFromCenter);
    float blendRate = saturate(edgeMask * gMaterial.intensity);
    output.color = float4(
        lerp(source.rgb, gMaterial.darkColor.rgb, blendRate),
        source.a);
    
    return output;
}
