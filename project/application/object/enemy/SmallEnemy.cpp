#define NOMINMAX
#include "SmallEnemy.h"

#include "BulletManager.h"
#include "DistanceFogManager.h"
#include "SplineCurve.h"

#include <algorithm>
#include <cmath>

void SmallEnemy::InitializeSpecific(const EnemyCreateConfig& config)
{
	config_ = config;
	role_ = config.role;
	if (role_ == SmallEnemyRole::Charger)
	{
		ModelManager::GetInstance()->LoadModel("enemy", "CrossEnemy.obj");
		GetObject3d()->SetModel("CrossEnemy.obj");
	}
	SetBodyCollisionDamage(role_ == SmallEnemyRole::Charger ? 0 : 1);

	spawnTargetPosition_ = config.position;
	Vector3 entryPosition = spawnTargetPosition_;
	entryPosition.z += DistanceFogManager::GetInstance()->GetEndDistance() +
		config_.spawnFogMargin;
	SetTranslate(entryPosition);
	ChangeState(SmallEnemyState::Entering);
}

void SmallEnemy::UpdateBehavior(const float& deltaTime)
{
	if (restoreBodyDamageNextUpdate_)
	{
		SetBodyCollisionDamage(1);
		restoreBodyDamageNextUpdate_ = false;
	}

	if (GetIsDead())
	{
		return;
	}
	if (state_ == SmallEnemyState::Entering ||
		state_ == SmallEnemyState::SpawnWait)
	{
		UpdateSpawnEntry(deltaTime);
		return;
	}

	if (role_ == SmallEnemyRole::Elite &&
		state_ == SmallEnemyState::Ranged &&
		!eliteChargeTriggered_ &&
		GetHitPoint() <= GetMaxHitPoint() / 2)
	{
		eliteChargeTriggered_ = true;
		ChangeState(SmallEnemyState::Charging);
		return;
	}

	switch (role_)
	{
	case SmallEnemyRole::Basic:
		if (state_ == SmallEnemyState::Escaping)
		{
			UpdateEscaping(deltaTime);
		}
		else
		{
			UpdateForwardMovement(deltaTime);
		}
		break;
	case SmallEnemyRole::Turret:
		break;
	case SmallEnemyRole::Charger:
		UpdateCharger(deltaTime);
		break;
	case SmallEnemyRole::Approacher:
		UpdateApproacher(deltaTime);
		break;
	case SmallEnemyRole::Elite:
		UpdateElite(deltaTime);
		break;
	}
}

void SmallEnemy::BeginRoleBehavior()
{
	if (role_ == SmallEnemyRole::Charger)
	{
		ChangeState(SmallEnemyState::Charging);
	}
	else
	{
		ChangeState(SmallEnemyState::Ranged);
	}
}

void SmallEnemy::UpdateSpawnEntry(float deltaTime)
{
	if (state_ == SmallEnemyState::Entering)
	{
		const Vector3 toTarget = spawnTargetPosition_ - GetTranslate();
		const float distance = Length(toTarget);
		const float movementDistance = config_.spawnApproachSpeed * deltaTime;
		if (distance <= movementDistance || distance <= 0.0001f ||
			config_.spawnApproachSpeed <= 0.0001f)
		{
			SetTranslate(spawnTargetPosition_);
			ChangeState(SmallEnemyState::SpawnWait);
			return;
		}

		SetTranslate(GetTranslate() + Normalize(toTarget) * movementDistance);
		return;
	}

	stateElapsedTime_ += deltaTime;
	if (stateElapsedTime_ >= config_.spawnSettleDuration)
	{
		BeginRoleBehavior();
	}
}

void SmallEnemy::UpdateAfterTransform()
{
	UpdateCameraFade();
	UpdateShooting(GetDeltaTime());
}

void SmallEnemy::UpdateDebugUI()
{
#ifdef _DEBUG
	ImGui::Text("Role: %s", GetRoleName());
	ImGui::Text("Behavior: %s", GetBehaviorStateName());
#endif
}

void SmallEnemy::OnPlayerCollision()
{
	if (state_ != SmallEnemyState::Rushing)
	{
		return;
	}

	if (role_ == SmallEnemyRole::Charger)
	{
		MarkForRemoval();
	}
	else if (role_ == SmallEnemyRole::Elite)
	{
		ChangeState(SmallEnemyState::NearCombat);
		restoreBodyDamageNextUpdate_ = true;
	}
}

void SmallEnemy::ChangeState(SmallEnemyState state)
{
	state_ = state;
	stateElapsedTime_ = 0.0f;
	stateStartPosition_ = GetTranslate();
	chargeReady_ = false;
	SetAttackChargeProgress(0.0f);
	if (state == SmallEnemyState::Escaping &&
		(role_ == SmallEnemyRole::Basic || role_ == SmallEnemyRole::Elite))
	{
		escapeDirectionX_ = GetTranslate().x >= 0.0f ? 1.0f : -1.0f;
	}

	if (state == SmallEnemyState::Charging ||
		state == SmallEnemyState::Escaping)
	{
		shotElapsedTime_ = 0.0f;
		burstElapsedTime_ = 0.0f;
		burstShotsRemaining_ = 0;
	}
}

void SmallEnemy::UpdateForwardMovement(float deltaTime)
{
	if (state_ != SmallEnemyState::Ranged)
	{
		return;
	}

	Vector3 position = GetTranslate();
	position.z = std::max(
		config_.nearLineZ, position.z - config_.forwardSpeed * deltaTime);
	SetTranslate(position);
	if (position.z <= config_.nearLineZ)
	{
		ChangeState(SmallEnemyState::Escaping);
	}
}

void SmallEnemy::UpdateCharger(float deltaTime)
{
	if (state_ == SmallEnemyState::Charging)
	{
		stateElapsedTime_ += deltaTime;
		const float progress = config_.chargeDuration > 0.0001f ?
			std::min(stateElapsedTime_ / config_.chargeDuration, 1.0f) : 1.0f;
		SetAttackChargeProgress(progress);
		if (progress >= 1.0f)
		{
			if (!chargeReady_)
			{
				chargeReady_ = true;
				return;
			}
			const Vector3 toPlayer = GetPlayerLocalPosition() - GetTranslate();
			if (Length(toPlayer) > 0.0001f)
			{
				rushDirection_ = Normalize(toPlayer);
			}
			rushTravelledDistance_ = 0.0f;
			SetBodyCollisionDamage(config_.contactDamage);
			ChangeState(SmallEnemyState::Rushing);
		}
		return;
	}

	if (state_ == SmallEnemyState::Rushing)
	{
		const Vector3 movement = rushDirection_ * config_.chargeSpeed * deltaTime;
		SetTranslate(GetTranslate() + movement);
		rushTravelledDistance_ += Length(movement);
		if (rushTravelledDistance_ >= config_.maxRushDistance)
		{
			MarkForRemoval();
		}
	}
}

void SmallEnemy::UpdateApproacher(float deltaTime)
{
	stateElapsedTime_ += deltaTime;
	switch (state_)
	{
	case SmallEnemyState::Ranged:
		if (stateElapsedTime_ >= config_.approachDelay)
		{
			stateTargetPosition_ = GetTranslate();
			stateTargetPosition_.z = std::max(
				config_.nearLineZ,
				stateTargetPosition_.z - config_.approachStepDistance);
			ChangeState(SmallEnemyState::Approaching);
		}
		break;
	case SmallEnemyState::Approaching:
	{
		const float progress = std::min(
			stateElapsedTime_ / std::max(config_.approachDuration, 0.0001f), 1.0f);
		const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
		SetTranslate(Lerp(stateStartPosition_, stateTargetPosition_, smoothProgress));
		if (progress >= 1.0f)
		{
			if (stateTargetPosition_.z <= config_.nearLineZ)
			{
				ChangeState(SmallEnemyState::NearCombat);
			}
			else
			{
				ChangeState(SmallEnemyState::Ranged);
			}
		}
		break;
	}
	case SmallEnemyState::NearCombat:
		if (stateElapsedTime_ >= config_.nearDuration)
		{
			ChangeState(SmallEnemyState::Escaping);
		}
		break;
	case SmallEnemyState::Escaping:
		UpdateEscaping(deltaTime);
		break;
	default:
		break;
	}
}

void SmallEnemy::UpdateElite(float deltaTime)
{
	if (state_ == SmallEnemyState::Ranged)
	{
		UpdateForwardMovement(deltaTime);
		return;
	}

	stateElapsedTime_ += deltaTime;
	switch (state_)
	{
	case SmallEnemyState::Charging:
	{
		const float progress = config_.chargeDuration > 0.0001f ?
			std::min(stateElapsedTime_ / config_.chargeDuration, 1.0f) : 1.0f;
		SetAttackChargeProgress(progress);
		if (progress >= 1.0f)
		{
			if (!chargeReady_)
			{
				chargeReady_ = true;
				break;
			}
			stateTargetPosition_ = GetPlayerLocalPosition();
			stateTargetPosition_.z = config_.nearLineZ;
			SetBodyCollisionDamage(config_.contactDamage);
			ChangeState(SmallEnemyState::Rushing);
		}
		break;
	}
	case SmallEnemyState::Rushing:
	{
		const float progress = std::min(
			stateElapsedTime_ / std::max(config_.rushDuration, 0.0001f), 1.0f);
		const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
		SetTranslate(Lerp(stateStartPosition_, stateTargetPosition_, smoothProgress));
		if (progress >= 1.0f)
		{
			SetBodyCollisionDamage(1);
			ChangeState(SmallEnemyState::NearCombat);
		}
		break;
	}
	case SmallEnemyState::NearCombat:
		if (stateElapsedTime_ >= config_.nearDuration)
		{
			ChangeState(SmallEnemyState::Escaping);
		}
		break;
	case SmallEnemyState::Escaping:
		UpdateEscaping(deltaTime);
		break;
	default:
		break;
	}
}

void SmallEnemy::UpdateEscaping(float deltaTime)
{
	Vector3 position = GetTranslate();
	const bool escapesDiagonally =
		role_ == SmallEnemyRole::Basic || role_ == SmallEnemyRole::Elite;
	if (escapesDiagonally)
	{
		position.x += escapeDirectionX_ * config_.escapeHorizontalSpeed * deltaTime;
	}
	position.z -= config_.escapeSpeed * deltaTime;
	SetTranslate(position);
	const bool outsideHorizontalBoundary = escapesDiagonally &&
		config_.despawnHorizontalX > 0.0f &&
		std::abs(position.x) >= config_.despawnHorizontalX;
	if (position.z <= config_.despawnLineZ || outsideHorizontalBoundary)
	{
		MarkForRemoval();
	}
}

void SmallEnemy::UpdateShooting(float deltaTime)
{
	if (GetIsDead() || !CanShootInCurrentState())
	{
		return;
	}

	if (burstShotsRemaining_ > 0)
	{
		burstElapsedTime_ += deltaTime;
		if (burstElapsedTime_ >= config_.burstShotInterval)
		{
			burstElapsedTime_ -= config_.burstShotInterval;
			FireProjectile();
			--burstShotsRemaining_;
			if (burstShotsRemaining_ == 0)
			{
				shotElapsedTime_ = 0.0f;
			}
		}
		return;
	}

	shotElapsedTime_ += deltaTime;
	if (shotElapsedTime_ < config_.shotInterval)
	{
		return;
	}

	shotElapsedTime_ -= config_.shotInterval;
	FireProjectile();
	if (UsesBurstFire())
	{
		burstShotsRemaining_ = 2;
		burstElapsedTime_ = 0.0f;
	}
}

void SmallEnemy::FireProjectile()
{
	Vector3 direction = { 0.0f, 0.0f, -1.0f };
	const Vector3 toPlayer = GetPlayerLocalPosition() - GetTranslate();
	if (Length(toPlayer) > 0.0001f)
	{
		direction = Normalize(toPlayer);
	}
	BulletManager::GetInstance()->CreateSmallEnemyBullet(
		GetTranslate(), direction * config_.bulletSpeed,
		GetParentWorldTransform());
}

bool SmallEnemy::UsesBurstFire() const
{
	return role_ == SmallEnemyRole::Turret || role_ == SmallEnemyRole::Elite;
}

bool SmallEnemy::CanShootInCurrentState() const
{
	switch (role_)
	{
	case SmallEnemyRole::Basic:
	case SmallEnemyRole::Turret:
		return state_ == SmallEnemyState::Ranged;
	case SmallEnemyRole::Approacher:
		return state_ == SmallEnemyState::Ranged ||
			state_ == SmallEnemyState::Approaching ||
			state_ == SmallEnemyState::NearCombat;
	case SmallEnemyRole::Elite:
		return state_ == SmallEnemyState::Ranged ||
			state_ == SmallEnemyState::NearCombat;
	case SmallEnemyRole::Charger:
		return false;
	}
	return false;
}
