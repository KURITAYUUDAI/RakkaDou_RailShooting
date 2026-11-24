#include "GPUParticle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);

struct EmitterSphere
{
	float32_t3 translate;		// 位置
	float32_t radius;			// 射出半径
	uint32_t count;				// 射出数
	float32_t frequency;		// 射出間隔
	float32_t frequencyTime;	// 射出間隔調整用時間
	uint32_t emit;				// 射出許可
};
ConstantBuffer<EmitterSphere> gEmitter : register(b0);

// 今回スレッド数は1。複数のEmitterを扱い、同時に処理したいような場合は適宜スレッド数を増やすと良い
[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if(gEmitter.emit != 0)
	{	// 射出許可が出たので射出
		for (uint32_t countIndex = 0; countIndex < gEmitter.count; ++countIndex)
		{
			// カウント分Particleを射出する
			gParticles[countIndex].scale = float32_t3(0.3f, 0.3f, 0.3f);
			gParticles[countIndex].translate = float32_t3(0.0f, 0.0f, 0.0f);
			gParticles[countIndex].color = float32_t4(1.0f, 0.0f, 0.0f, 1.0f);
		}
	}
}