#define NOMINMAX
#include "SmallEnemy.h"

#include "BulletManager.h"
#include "DistanceFogManager.h"
#include "SplineCurve.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

class ISmallEnemyState
{
public:
	virtual ~ISmallEnemyState() = default;

	virtual void Enter(SmallEnemy&) {}
	virtual std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) = 0;
	virtual bool CanShoot() const { return false; }
	virtual std::optional<SmallEnemyState> OnPlayerCollision(SmallEnemy&)
	{
		return std::nullopt;
	}
	virtual float GetRailFollowBlend(const SmallEnemy&) const { return 0.0f; }
};

class ISmallEnemyRoleBehavior
{
public:
	virtual ~ISmallEnemyRoleBehavior() = default;

	virtual void Initialize(SmallEnemy& enemy) = 0;
	virtual SmallEnemyState GetInitialState() const = 0;
	virtual std::optional<SmallEnemyState> UpdateRanged(
		SmallEnemy&, float) { return std::nullopt; }
	virtual std::optional<SmallEnemyState> UpdateCharging(
		SmallEnemy&, float) { return std::nullopt; }
	virtual std::optional<SmallEnemyState> UpdateRushing(
		SmallEnemy&, float) { return std::nullopt; }
	virtual std::optional<SmallEnemyState> UpdateApproaching(
		SmallEnemy&, float) { return std::nullopt; }
	virtual std::optional<SmallEnemyState> OnPlayerCollision(SmallEnemy&)
	{
		return std::nullopt;
	}
	virtual bool UsesBurstFire() const = 0;
	virtual bool EscapesDiagonally() const { return false; }
};

class BasicSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	SmallEnemyState GetInitialState() const override
	{
		return SmallEnemyState::Ranged;
	}

	std::optional<SmallEnemyState> UpdateRanged(
		SmallEnemy& enemy, float deltaTime) override
	{
		return enemy.UpdateForwardMovement(deltaTime)
			? std::optional{ SmallEnemyState::Escaping }
			: std::nullopt;
	}

	bool UsesBurstFire() const override { return false; }
	bool EscapesDiagonally() const override { return true; }
};

class TurretSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	SmallEnemyState GetInitialState() const override
	{
		return SmallEnemyState::Ranged;
	}

	bool UsesBurstFire() const override { return true; }
};

class ChargerSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		ModelManager::GetInstance()->LoadModel("enemy", "CrossEnemy.obj");
		enemy.GetObject3d()->SetModel("CrossEnemy.obj");
		enemy.SetBodyCollisionDamage(0);
	}

	SmallEnemyState GetInitialState() const override
	{
		return SmallEnemyState::Charging;
	}

	std::optional<SmallEnemyState> UpdateCharging(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		const float progress = enemy.config_.chargeDuration > 0.0001f
			? std::min(
				enemy.stateElapsedTime_ / enemy.config_.chargeDuration, 1.0f)
			: 1.0f;
		enemy.SetAttackChargeProgress(progress);
		if (progress < 1.0f)
		{
			return std::nullopt;
		}
		if (!enemy.chargeReady_)
		{
			enemy.chargeReady_ = true;
			return std::nullopt;
		}

		const Vector3 toPlayer =
			enemy.GetPlayerLocalPosition() - enemy.GetTranslate();
		if (Length(toPlayer) > 0.0001f)
		{
			enemy.rushDirection_ = Normalize(toPlayer);
		}
		enemy.rushTravelledDistance_ = 0.0f;
		enemy.SetBodyCollisionDamage(enemy.config_.contactDamage);
		return SmallEnemyState::Rushing;
	}

	std::optional<SmallEnemyState> UpdateRushing(
		SmallEnemy& enemy, float deltaTime) override
	{
		const Vector3 currentPosition = enemy.GetTranslate();
		const Vector3 toPlayer =
			enemy.GetPlayerLocalPosition() - currentPosition;
		const float movementDistance = enemy.config_.chargeSpeed * deltaTime;
		const float remainingAlongRush = Dot(toPlayer, enemy.rushDirection_);
		const Vector3 lateralOffset =
			toPlayer - enemy.rushDirection_ * remainingAlongRush;
		const float stopRadius = enemy.config_.colliderRadius + 1.0f;
		const bool reachesPlayerPlane =
			remainingAlongRush >= 0.0f && remainingAlongRush <= movementDistance;
		const bool passesCloseToPlayer =
			reachesPlayerPlane && Length(lateralOffset) <= stopRadius;
		const float appliedDistance =
			passesCloseToPlayer ? remainingAlongRush : movementDistance;
		const Vector3 movement = enemy.rushDirection_ * appliedDistance;
		enemy.SetTranslate(enemy.GetTranslate() + movement);
		enemy.rushTravelledDistance_ += Length(movement);
		if (passesCloseToPlayer ||
			enemy.rushTravelledDistance_ >= enemy.config_.maxRushDistance)
		{
			enemy.MarkForRemoval();
		}
		return std::nullopt;
	}

	std::optional<SmallEnemyState> OnPlayerCollision(
		SmallEnemy& enemy) override
	{
		enemy.MarkForRemoval();
		return std::nullopt;
	}

	bool UsesBurstFire() const override { return false; }
};

class ApproacherSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	SmallEnemyState GetInitialState() const override
	{
		return SmallEnemyState::Ranged;
	}

	std::optional<SmallEnemyState> UpdateRanged(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		if (enemy.stateElapsedTime_ < enemy.config_.approachDelay)
		{
			return std::nullopt;
		}

		enemy.stateTargetPosition_ = enemy.GetTranslate();
		enemy.stateTargetPosition_.z = std::max(
			enemy.config_.nearLineZ,
			enemy.stateTargetPosition_.z - enemy.config_.approachStepDistance);
		return SmallEnemyState::Approaching;
	}

	std::optional<SmallEnemyState> UpdateApproaching(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		const float progress = std::min(
			enemy.stateElapsedTime_ /
				std::max(enemy.config_.approachDuration, 0.0001f),
			1.0f);
		const float smoothProgress =
			progress * progress * (3.0f - 2.0f * progress);
		enemy.SetTranslate(Lerp(
			enemy.stateStartPosition_, enemy.stateTargetPosition_, smoothProgress));
		if (progress < 1.0f)
		{
			return std::nullopt;
		}
		return enemy.stateTargetPosition_.z <= enemy.config_.nearLineZ
			? SmallEnemyState::NearCombat
			: SmallEnemyState::Ranged;
	}

	bool UsesBurstFire() const override { return false; }
};

class EliteSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	SmallEnemyState GetInitialState() const override
	{
		return SmallEnemyState::Ranged;
	}

	std::optional<SmallEnemyState> UpdateRanged(
		SmallEnemy& enemy, float deltaTime) override
	{
		if (!enemy.eliteChargeTriggered_ &&
			enemy.GetHitPoint() <= enemy.GetMaxHitPoint() / 2)
		{
			enemy.eliteChargeTriggered_ = true;
			return SmallEnemyState::Charging;
		}

		return enemy.UpdateForwardMovement(deltaTime)
			? std::optional{ SmallEnemyState::Escaping }
			: std::nullopt;
	}

	std::optional<SmallEnemyState> UpdateCharging(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		const float progress = enemy.config_.chargeDuration > 0.0001f
			? std::min(
				enemy.stateElapsedTime_ / enemy.config_.chargeDuration, 1.0f)
			: 1.0f;
		enemy.SetAttackChargeProgress(progress);
		if (progress < 1.0f)
		{
			return std::nullopt;
		}
		if (!enemy.chargeReady_)
		{
			enemy.chargeReady_ = true;
			return std::nullopt;
		}

		enemy.stateTargetPosition_ = enemy.GetPlayerLocalPosition();
		enemy.stateTargetPosition_.z = enemy.config_.nearLineZ;
		enemy.SetBodyCollisionDamage(enemy.config_.contactDamage);
		return SmallEnemyState::Rushing;
	}

	std::optional<SmallEnemyState> UpdateRushing(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		const float progress = std::min(
			enemy.stateElapsedTime_ /
				std::max(enemy.config_.rushDuration, 0.0001f),
			1.0f);
		const float smoothProgress =
			progress * progress * (3.0f - 2.0f * progress);
		enemy.SetTranslate(Lerp(
			enemy.stateStartPosition_, enemy.stateTargetPosition_, smoothProgress));
		if (progress < 1.0f)
		{
			return std::nullopt;
		}

		enemy.SetBodyCollisionDamage(1);
		return SmallEnemyState::NearCombat;
	}

	std::optional<SmallEnemyState> OnPlayerCollision(
		SmallEnemy& enemy) override
	{
		enemy.restoreBodyDamageNextUpdate_ = true;
		return SmallEnemyState::NearCombat;
	}

	bool UsesBurstFire() const override { return true; }
	bool EscapesDiagonally() const override { return true; }
};

class SmallEnemyEnteringState final : public ISmallEnemyState
{
public:
	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		const Vector3 toTarget =
			enemy.spawnTargetPosition_ - enemy.GetTranslate();
		const float distance = Length(toTarget);
		const float movementDistance =
			enemy.config_.spawnApproachSpeed * deltaTime;
		if (distance <= movementDistance || distance <= 0.0001f ||
			enemy.config_.spawnApproachSpeed <= 0.0001f)
		{
			enemy.SetTranslate(enemy.spawnTargetPosition_);
			return SmallEnemyState::SpawnWait;
		}

		enemy.SetTranslate(
			enemy.GetTranslate() + Normalize(toTarget) * movementDistance);
		return std::nullopt;
	}

	float GetRailFollowBlend(const SmallEnemy& enemy) const override
	{
		const float totalDistance =
			Length(enemy.spawnTargetPosition_ - enemy.stateStartPosition_);
		if (totalDistance <= 0.0001f)
		{
			return 0.0f;
		}
		const float remainingRate = std::clamp(
			Length(enemy.spawnTargetPosition_ - enemy.GetTranslate()) /
				totalDistance,
			0.0f, 1.0f);
		return remainingRate * remainingRate *
			(3.0f - 2.0f * remainingRate);
	}
};

class SmallEnemySpawnWaitState final : public ISmallEnemyState
{
public:
	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		if (enemy.stateElapsedTime_ >= enemy.config_.spawnSettleDuration)
		{
			return enemy.GetInitialRoleState();
		}
		return std::nullopt;
	}
};

class SmallEnemyRangedState final : public ISmallEnemyState
{
public:
	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		return enemy.roleBehavior_->UpdateRanged(enemy, deltaTime);
	}

	bool CanShoot() const override { return true; }
};

class SmallEnemyChargingState final : public ISmallEnemyState
{
public:
	void Enter(SmallEnemy& enemy) override
	{
		enemy.shotElapsedTime_ = 0.0f;
		enemy.burstElapsedTime_ = 0.0f;
		enemy.burstShotsRemaining_ = 0;
	}

	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		return enemy.roleBehavior_->UpdateCharging(enemy, deltaTime);
	}
};

class SmallEnemyRushingState final : public ISmallEnemyState
{
public:
	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		return enemy.roleBehavior_->UpdateRushing(enemy, deltaTime);
	}

	std::optional<SmallEnemyState> OnPlayerCollision(
		SmallEnemy& enemy) override
	{
		return enemy.roleBehavior_->OnPlayerCollision(enemy);
	}
};

class SmallEnemyApproachingState final : public ISmallEnemyState
{
public:
	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		return enemy.roleBehavior_->UpdateApproaching(enemy, deltaTime);
	}

	bool CanShoot() const override { return true; }
};

class SmallEnemyNearCombatState final : public ISmallEnemyState
{
public:
	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.stateElapsedTime_ += deltaTime;
		if (enemy.stateElapsedTime_ >= enemy.config_.nearDuration)
		{
			return SmallEnemyState::Escaping;
		}
		return std::nullopt;
	}

	bool CanShoot() const override { return true; }
};

class SmallEnemyEscapingState final : public ISmallEnemyState
{
public:
	void Enter(SmallEnemy& enemy) override
	{
		if (enemy.roleBehavior_->EscapesDiagonally())
		{
			enemy.escapeDirectionX_ =
				enemy.GetTranslate().x >= 0.0f ? 1.0f : -1.0f;
		}
		enemy.shotElapsedTime_ = 0.0f;
		enemy.burstElapsedTime_ = 0.0f;
		enemy.burstShotsRemaining_ = 0;
	}

	std::optional<SmallEnemyState> Update(
		SmallEnemy& enemy, float deltaTime) override
	{
		enemy.UpdateEscaping(deltaTime);
		return std::nullopt;
	}
};

namespace
{
	std::unique_ptr<ISmallEnemyState> CreateStateBehavior(
		SmallEnemyState state)
	{
		switch (state)
		{
		case SmallEnemyState::Entering:
			return std::make_unique<SmallEnemyEnteringState>();
		case SmallEnemyState::SpawnWait:
			return std::make_unique<SmallEnemySpawnWaitState>();
		case SmallEnemyState::Ranged:
			return std::make_unique<SmallEnemyRangedState>();
		case SmallEnemyState::Charging:
			return std::make_unique<SmallEnemyChargingState>();
		case SmallEnemyState::Rushing:
			return std::make_unique<SmallEnemyRushingState>();
		case SmallEnemyState::Approaching:
			return std::make_unique<SmallEnemyApproachingState>();
		case SmallEnemyState::NearCombat:
			return std::make_unique<SmallEnemyNearCombatState>();
		case SmallEnemyState::Escaping:
			return std::make_unique<SmallEnemyEscapingState>();
		}
		return nullptr;
	}

	std::unique_ptr<ISmallEnemyRoleBehavior> CreateRoleBehavior(SmallEnemyRole role)
	{
		switch (role)
		{
		case SmallEnemyRole::Basic:
			return std::make_unique<BasicSmallEnemyBehavior>();
		case SmallEnemyRole::Turret:
			return std::make_unique<TurretSmallEnemyBehavior>();
		case SmallEnemyRole::Charger:
			return std::make_unique<ChargerSmallEnemyBehavior>();
		case SmallEnemyRole::Approacher:
			return std::make_unique<ApproacherSmallEnemyBehavior>();
		case SmallEnemyRole::Elite:
			return std::make_unique<EliteSmallEnemyBehavior>();
		}
		return std::make_unique<BasicSmallEnemyBehavior>();
	}
}

SmallEnemy::SmallEnemy() = default;
SmallEnemy::~SmallEnemy() = default;

void SmallEnemy::InitializeSpecific(const EnemyCreateConfig& config)
{
	config_ = config;
	role_ = config.role;
	roleBehavior_ = CreateRoleBehavior(role_);
	roleBehavior_->Initialize(*this);

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
	if (!stateBehavior_)
	{
		return;
	}

	const std::optional<SmallEnemyState> nextState =
		stateBehavior_->Update(*this, deltaTime);
	if (nextState)
	{
		ChangeState(*nextState);
	}
}

SmallEnemyState SmallEnemy::GetInitialRoleState() const
{
	return roleBehavior_->GetInitialState();
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
	ImGui::Text(
		"Continuous Rail Follow: %s",
		IsContinuousRailFollowEnabled() ? "ON" : "OFF");
#endif
}

float SmallEnemy::GetRailFollowBlend() const
{
	// 新しい連続追従モードでは、登場後の戦闘中も論理座標を
	// レール上の位置と姿勢へ完全に変換する。
	if (IsContinuousRailFollowEnabled())
	{
		return 1.0f;
	}

	return stateBehavior_ ? stateBehavior_->GetRailFollowBlend(*this) : 0.0f;
}

void SmallEnemy::OnPlayerCollision()
{
	if (stateBehavior_)
	{
		const std::optional<SmallEnemyState> nextState =
			stateBehavior_->OnPlayerCollision(*this);
		if (nextState)
		{
			ChangeState(*nextState);
		}
	}
}

void SmallEnemy::ChangeState(SmallEnemyState state)
{
	state_ = state;
	stateBehavior_ = CreateStateBehavior(state);
	stateElapsedTime_ = 0.0f;
	stateStartPosition_ = GetTranslate();
	chargeReady_ = false;
	SetAttackChargeProgress(0.0f);
	if (stateBehavior_)
	{
		stateBehavior_->Enter(*this);
	}
}

bool SmallEnemy::UpdateForwardMovement(float deltaTime)
{
	Vector3 position = GetTranslate();
	position.z = std::max(
		config_.nearLineZ, position.z - config_.forwardSpeed * deltaTime);
	SetTranslate(position);
	return position.z <= config_.nearLineZ;
}

void SmallEnemy::UpdateEscaping(float deltaTime)
{
	Vector3 position = GetTranslate();
	const bool escapesDiagonally = roleBehavior_->EscapesDiagonally();
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
	const Vector3 worldPosition = GetWorldPosition();
	Vector3 worldDirection = { 0.0f, 0.0f, -1.0f };
	const Vector3 toPlayer = GetPlayerWorldPosition() - worldPosition;
	if (Length(toPlayer) > 0.0001f)
	{
		worldDirection = Normalize(toPlayer);
	}

	const WorldTransform* parent = GetParentWorldTransform();
	Vector3 localPosition = worldPosition;
	Vector3 localDirection = worldDirection;
	if (parent)
	{
		const Matrix4x4 inverseParent = Inverse(parent->GetWorldMatrix());
		localPosition = TransformPosition(worldPosition, inverseParent);
		localDirection = TransformNormal(worldDirection, inverseParent);
		if (Length(localDirection) > 0.0001f)
		{
			localDirection = Normalize(localDirection);
		}
	}
	BulletManager::GetInstance()->CreateSmallEnemyBullet(
		localPosition, localDirection * config_.bulletSpeed, parent);
}

bool SmallEnemy::UsesBurstFire() const
{
	return roleBehavior_->UsesBurstFire();
}

bool SmallEnemy::CanShootInCurrentState() const
{
	return stateBehavior_ && stateBehavior_->CanShoot();
}
