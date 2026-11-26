#define NOMINMAX
#include "SmallEnemy.h"

#include "BulletManager.h"
#include "DistanceFogManager.h"
#include "SplineCurve.h"

#include <algorithm>
#include <cmath>
#include <memory>

class ISmallEnemyRoleBehavior
{
public:
	virtual ~ISmallEnemyRoleBehavior() = default;

	virtual void Initialize(SmallEnemy& enemy) = 0;
	virtual void Begin(SmallEnemy& enemy) = 0;
	virtual void Update(SmallEnemy& enemy, float deltaTime) = 0;
	virtual void OnPlayerCollision(SmallEnemy& enemy) {}
	virtual bool UsesBurstFire() const = 0;
	virtual bool CanShoot(SmallEnemyState state) const = 0;
	virtual bool EscapesDiagonally() const { return false; }
};

class BasicSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	void Begin(SmallEnemy& enemy) override
	{
		enemy.ChangeState(SmallEnemyState::Ranged);
	}

	void Update(SmallEnemy& enemy, float deltaTime) override
	{
		if (enemy.state_ == SmallEnemyState::Escaping)
		{
			enemy.UpdateEscaping(deltaTime);
		}
		else
		{
			enemy.UpdateForwardMovement(deltaTime);
		}
	}

	bool UsesBurstFire() const override { return false; }
	bool CanShoot(SmallEnemyState state) const override
	{
		return state == SmallEnemyState::Ranged;
	}
	bool EscapesDiagonally() const override { return true; }
};

class TurretSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	void Begin(SmallEnemy& enemy) override
	{
		enemy.ChangeState(SmallEnemyState::Ranged);
	}

	void Update(SmallEnemy&, float) override {}
	bool UsesBurstFire() const override { return true; }
	bool CanShoot(SmallEnemyState state) const override
	{
		return state == SmallEnemyState::Ranged;
	}
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

	void Begin(SmallEnemy& enemy) override
	{
		enemy.ChangeState(SmallEnemyState::Charging);
	}

	void Update(SmallEnemy& enemy, float deltaTime) override
	{
		enemy.UpdateCharger(deltaTime);
	}

	void OnPlayerCollision(SmallEnemy& enemy) override
	{
		enemy.MarkForRemoval();
	}

	bool UsesBurstFire() const override { return false; }
	bool CanShoot(SmallEnemyState) const override { return false; }
};

class ApproacherSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	void Begin(SmallEnemy& enemy) override
	{
		enemy.ChangeState(SmallEnemyState::Ranged);
	}

	void Update(SmallEnemy& enemy, float deltaTime) override
	{
		enemy.UpdateApproacher(deltaTime);
	}

	bool UsesBurstFire() const override { return false; }
	bool CanShoot(SmallEnemyState state) const override
	{
		return state == SmallEnemyState::Ranged ||
			state == SmallEnemyState::Approaching ||
			state == SmallEnemyState::NearCombat;
	}
};

class EliteSmallEnemyBehavior final : public ISmallEnemyRoleBehavior
{
public:
	void Initialize(SmallEnemy& enemy) override
	{
		enemy.SetBodyCollisionDamage(1);
	}

	void Begin(SmallEnemy& enemy) override
	{
		enemy.ChangeState(SmallEnemyState::Ranged);
	}

	void Update(SmallEnemy& enemy, float deltaTime) override
	{
		if (enemy.state_ == SmallEnemyState::Ranged &&
			!enemy.eliteChargeTriggered_ &&
			enemy.GetHitPoint() <= enemy.GetMaxHitPoint() / 2)
		{
			enemy.eliteChargeTriggered_ = true;
			enemy.ChangeState(SmallEnemyState::Charging);
			return;
		}

		enemy.UpdateElite(deltaTime);
	}

	void OnPlayerCollision(SmallEnemy& enemy) override
	{
		enemy.ChangeState(SmallEnemyState::NearCombat);
		enemy.restoreBodyDamageNextUpdate_ = true;
	}

	bool UsesBurstFire() const override { return true; }
	bool CanShoot(SmallEnemyState state) const override
	{
		return state == SmallEnemyState::Ranged ||
			state == SmallEnemyState::NearCombat;
	}
	bool EscapesDiagonally() const override { return true; }
};

namespace
{
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
	if (state_ == SmallEnemyState::Entering ||
		state_ == SmallEnemyState::SpawnWait)
	{
		UpdateSpawnEntry(deltaTime);
		return;
	}

	roleBehavior_->Update(*this, deltaTime);
}

void SmallEnemy::BeginRoleBehavior()
{
	roleBehavior_->Begin(*this);
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

	// フラグがOFFの場合は、従来の登場中だけブレンドする振る舞いを保つ。
	if (state_ == SmallEnemyState::Entering)
	{
		const float totalDistance =
			Length(spawnTargetPosition_ - stateStartPosition_);
		if (totalDistance <= 0.0001f)
		{
			return 0.0f;
		}
		const float remainingRate = std::clamp(
			Length(spawnTargetPosition_ - GetTranslate()) / totalDistance,
			0.0f, 1.0f);
		// 登場移動と同時にレール姿勢からバトルスペース姿勢へ移行する。
		// 両端の速度を0にして、切り替えが横滑りとして見えないようにする。
		return remainingRate * remainingRate *
			(3.0f - 2.0f * remainingRate);
	}
	return 0.0f;
}

void SmallEnemy::OnPlayerCollision()
{
	if (state_ != SmallEnemyState::Rushing)
	{
		return;
	}

	roleBehavior_->OnPlayerCollision(*this);
}

void SmallEnemy::ChangeState(SmallEnemyState state)
{
	state_ = state;
	stateElapsedTime_ = 0.0f;
	stateStartPosition_ = GetTranslate();
	chargeReady_ = false;
	SetAttackChargeProgress(0.0f);
	if (state == SmallEnemyState::Escaping &&
		roleBehavior_->EscapesDiagonally())
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
		const Vector3 currentPosition = GetTranslate();
		const Vector3 toPlayer = GetPlayerLocalPosition() - currentPosition;
		const float movementDistance = config_.chargeSpeed * deltaTime;
		const float remainingAlongRush = Dot(toPlayer, rushDirection_);
		const Vector3 lateralOffset =
			toPlayer - rushDirection_ * remainingAlongRush;
		const float stopRadius = config_.colliderRadius + 1.0f;
		const bool reachesPlayerPlane =
			remainingAlongRush >= 0.0f && remainingAlongRush <= movementDistance;
		const bool passesCloseToPlayer =
			reachesPlayerPlane && Length(lateralOffset) <= stopRadius;
		const float appliedDistance = passesCloseToPlayer ?
			remainingAlongRush : movementDistance;
		const Vector3 movement = rushDirection_ * appliedDistance;
		SetTranslate(GetTranslate() + movement);
		rushTravelledDistance_ += Length(movement);
		if (passesCloseToPlayer ||
			rushTravelledDistance_ >= config_.maxRushDistance)
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
	return roleBehavior_->CanShoot(state_);
}
