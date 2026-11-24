#ifndef GPU_PARTICLE_HLSLI
#define GPU_PARTICLE_HLSLI

struct EulerTransform
{
	float32_t3 scale;
	float32_t3 rotate;
	float32_t3 translate;
};

struct GPUParticle
{
	float32_t3 translate; // 位置
	float32_t3 scale; // 拡縮
	//EulerTransform transform; // 変換情報
	//EulerTransform uvTransform = { { 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } };
							  // UV変換情報
	float32_t3 velocity; // 速度
	float32_t4 color; // 色
	//float initialAlpha = 1.0f; // 寿命フェード前の不透明度
	float lifeTime; // 生存時間
	float currentTime; // 経過時間
	
	//ParticleParentHandle parentHandle;
};

struct ParticleSimulationConstants
{
	float32_t deltaTime;
	uint32_t maxParticleCount;
	float32_t totalTime;
	uint32_t initialAliveCount;
};

#endif