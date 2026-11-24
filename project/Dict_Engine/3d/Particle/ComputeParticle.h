#pragma once
#define NOMINMAX
#include "DirectXBase.h"
#include "../../math/myMath.h"

#include <memory>
#include <vector>

#include "PSOManager.h"

#include "MaterialInstance.h"

#include <limits>

class Model;
class Camera;

class SrvManager;

struct ParticleParentHandle;

struct GPUParticle
{
	Vector3 translate;	// 位置
	Vector3 scale;		// 拡縮
	//EulerTransform transform;	// 変換情報
	//EulerTransform uvTransform = { { 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } };
	// UV変換情報
	Vector3 velocity;		// 速度
	Vector4 color;			// 色
	//float initialAlpha = 1.0f; // 寿命フェード前の不透明度
	float lifeTime;			// 生存時間
	float currentTime;		// 経過時間
	uint32_t isAlive;		// 生存フラグ
	//ParticleParentHandle parentHandle;
};

struct PerView
{
	Matrix4x4 viewProjection;
	Matrix4x4 billboardMatrix;
};

struct ParticleSimulationConstants
{
	float deltaTime;
	uint32_t maxParticleCount;
	float totalTime;
	uint32_t initialAliveCount;
};

class ComputeParticle
{
public:
	
	void Initialize(uint32_t maxParticleCount = 1024);

	void DispatchInitialize();
	
public: // 外部入出力

	uint32_t GetParticleCSSrvIndex() const { return particleCSSrvIndex_; }
	uint32_t GetParticleCSUavIndex() const { return particleCSUavIndex_; }
	uint32_t GetMaxParticleCount() const { return maxParticleCount_; }

private:
	void RegisterInitializePSO();
	void CreateParticleCSResource();
	void TransitionParticleResource(D3D12_RESOURCE_STATES nextState);
private:

	uint32_t maxParticleCount_ = 1024;

	Microsoft::WRL::ComPtr<ID3D12Resource> particleCSResource_ = nullptr;
	uint32_t particleCSSrvIndex_ = 0;
	uint32_t particleCSUavIndex_ = 0;
	D3D12_RESOURCE_STATES particleState_ = D3D12_RESOURCE_STATE_COMMON;

	static constexpr const char* kInitializePsoName = "InitializeParticle";
	static constexpr const char* kUpdatePsoName = "UpdateParticle";
};

