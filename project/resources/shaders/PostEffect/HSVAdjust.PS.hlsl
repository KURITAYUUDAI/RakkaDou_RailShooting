#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float hueShift;
    float saturation;
    float value;
    float intensity;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float3 RGBToHSV(float3 color)
{
    const float4 k = float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    const float4 p = lerp(
        float4(color.bg, k.wz),
        float4(color.gb, k.xy),
        step(color.b, color.g));
    const float4 q = lerp(
        float4(p.xyw, color.r),
        float4(color.r, p.yzx),
        step(p.x, color.r));
    const float delta = q.x - min(q.w, q.y);
    const float epsilon = 1.0e-10f;
    return float3(
        abs(q.z + (q.w - q.y) / (6.0f * delta + epsilon)),
        delta / (q.x + epsilon),
        q.x);
}

float3 HSVToRGB(float3 hsv)
{
    const float3 rgb = saturate(
        abs(frac(hsv.xxx + float3(0.0f, 2.0f / 3.0f, 1.0f / 3.0f)) *
            6.0f - 3.0f) - 1.0f);
    return hsv.z * lerp(float3(1.0f, 1.0f, 1.0f), rgb, hsv.y);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    const float4 source = gTexture.Sample(gSampler, input.texcoord);
    float3 hsv = RGBToHSV(source.rgb);
    hsv.x = frac(hsv.x + gMaterial.hueShift + 1.0f);
    hsv.y = saturate(hsv.y * max(gMaterial.saturation, 0.0f));
    hsv.z *= max(gMaterial.value, 0.0f);

    const float3 adjusted = HSVToRGB(hsv);
    output.color = float4(
        lerp(source.rgb, adjusted, saturate(gMaterial.intensity)),
        source.a);
    return output;
}
