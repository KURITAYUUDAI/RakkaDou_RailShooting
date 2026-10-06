#pragma once
#include "myMath.h"
#include "DirectXBase.h"

class ParticleEmitter
{
public:

	void Initialize(const std::string& name, const EulerTransform& transform, const uint32_t& count, const float& frequency);

	void Update(const float& deltaTime);

	void Emit();

	void RandomEmit(const float& deltaTime);

	void EmitSlash();

	void EmitChargeAbsorb(const float& deltaTime);

public: // 外部入出力

	void SetName(const std::string& name){ name_ = name; }
	void SetTranslate(const Vector3& translate){ transform_.translate = translate; }
	void SetCount(const uint32_t& count){ count_ = count; }
	void SetFrequency(const float& frequency){ frequency_ = frequency; }

	const std::string& GetName() const { return name_; }
	const Vector3& GetTranslate() const { return transform_.translate; }
	const uint32_t& GetCount() const { return count_; }
	const float& GetFrequency() const { return frequency_; }

private:

	float deltaTime_ = 0.0f;

	std::string name_;

	EulerTransform transform_;

	uint32_t count_;

	float frequency_;

	float frequencyTime_;
};

