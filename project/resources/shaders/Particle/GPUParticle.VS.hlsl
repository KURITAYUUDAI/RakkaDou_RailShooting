#include "Particle.hlsli"
#include "GPUParticle.hlsli"

struct VertexShaderInput
{
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
	float4 color : COLOR0;
};

struct PerView
{
	row_major float32_t4x4 viewProjection;
	row_major float32_t4x4 billboardMatrix;
};

StructuredBuffer<GPUParticle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
	VertexShaderOutput output;
	GPUParticle particle = gParticles[instanceId];
	float32_t4x4 worldMatrix = gPerView.billboardMatrix;
	worldMatrix[0] *= particle.scale.x;
	worldMatrix[1] *= particle.scale.y;
	worldMatrix[2] *= particle.scale.z;
	worldMatrix[3].xyz = particle.translate;
	
	output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
	output.texcoord = input.texcoord;
	//output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
	output.normal = float32_t3(0.0, 0.0, 1.0);
	output.color = particle.color * input.color;
		output.uvTransform = 
	float32_t4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
		
		
	return output;
}