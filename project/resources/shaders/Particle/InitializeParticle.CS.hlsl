#include "GPUParticle.hlsli"

static const uint32_t kMaxParticleCount = 1024;

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
//ConstantBuffer<ParticleSimulationConstants> gConstants : register(b0);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint32_t particleIndex = DTid.x;
	if(particleIndex < kMaxParticleCount)
	{
		// Particle構造体の全要素を0で埋めるという書き方
		gParticles[particleIndex] = (GPUParticle) 0;
	}
}