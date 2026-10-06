#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "time/DeltaTimeManager.h"

void ParticleEmitter::Initialize(const std::string& name, const EulerTransform& transform, 
	const uint32_t& count, const float& frequency)
{
	name_ = name;
	transform_ = transform;
	count_ = count;
	frequency_ = frequency;
	frequencyTime_ = 0.0f;
}

void ParticleEmitter::Update(const float& deltaTime)
{
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Effect);

	frequencyTime_ += deltaTime_;
	if (frequency_ <= frequencyTime_)
	{
		ParticleManager::GetInstance()->RandomEmit(name_, transform_.translate, count_);
		frequencyTime_ -= frequency_;
	}
}

void ParticleEmitter::Emit()
{
	ParticleManager::GetInstance()->Emit(name_, transform_.translate, count_);
}

void ParticleEmitter::RandomEmit(const float& deltaTime)
{
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Effect);

	frequencyTime_ += deltaTime_;
	if (frequency_ <= frequencyTime_)
	{
		ParticleManager::GetInstance()->RandomEmit(name_, transform_.translate, count_);
		frequencyTime_ -= frequency_;
	}
}

void ParticleEmitter::EmitSlash()
{
	ParticleManager::GetInstance()->EmitSlash(name_, transform_.translate, count_);
}

void ParticleEmitter::EmitChargeAbsorb(const float& deltaTime)
{
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Effect);

	frequencyTime_ += deltaTime_;
	while (frequency_ <= frequencyTime_)
	{
		ParticleManager::GetInstance()->EmitChargeAbsorb(
			name_, transform_.translate, count_);
		frequencyTime_ -= frequency_;
	}
}

