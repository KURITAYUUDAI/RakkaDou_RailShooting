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
	//ParticleParentHandle parentHandle;
};

struct PerView
{
	Matrix4x4 viewProjection;
	Matrix4x4 billboardMatrix;
};

struct EmitterSphere
{
	Vector3 translate;		// 位置
	float radius;			// 射出半径
	uint32_t count;			// 射出数
	float frequency;		// 射出間隔
	float frequencyTime;	// 射出間隔調整用時間
	uint32_t emit;			// 射出許可
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

	void BeginUAVWrite()
	{
		TransitionParticleResource(
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	void EndUAVWrite()
	{
		TransitionParticleResource(
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

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

class ComputeEmitter
{
public:

	void Initialize(const std::string& name, const Vector3& translate, 
		const uint32_t& count, const float& frequency);

	void Update(const float& deltaTime);

	void Dispatch(uint32_t particleCSUavIndex);

	/*void Emit();

	void RandomEmit(const float& deltaTime);

	void EmitSlash(
		const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
		float length = 2.0f, float lifeTime = 0.25f);*/

public: // 外部入出力

	/*void SetName(const std::string& name){ name_ = name; }
	void SetTranslate(const Vector3& translate){ transform_.translate = translate; }
	void SetCount(const uint32_t& count){ count_ = count; }
	void SetFrequency(const float& frequency){ frequency_ = frequency; }
	void SetParentHandle(ParticleParentHandle parentHandle)
	{
		parentHandle_ = parentHandle;
	}
	void ClearParentHandle() { parentHandle_ = {}; }

	const std::string& GetName() const { return name_; }
	const Vector3& GetTranslate() const { return transform_.translate; }
	const uint32_t& GetCount() const { return count_; }
	const float& GetFrequency() const { return frequency_; }*/

private:

	void RegisterEmitPSO();
	void CreateEmitterResource();
	void TransitionEmitterResource(D3D12_RESOURCE_STATES nextState);

private:

	float deltaTime_ = 0.0f;

	std::string name_;

	EmitterSphere* emitterSphere_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> emitterResource_ = nullptr;
	D3D12_RESOURCE_STATES emitterState_ = D3D12_RESOURCE_STATE_COMMON;

	static constexpr const char* kPsoName = "EmitParticle";
};
