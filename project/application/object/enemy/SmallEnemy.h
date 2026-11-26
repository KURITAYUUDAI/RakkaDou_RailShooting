#pragma once

#include "Enemy.h"

class ISmallEnemyRoleBehavior;
class ISmallEnemyState;
class BasicSmallEnemyBehavior;
class TurretSmallEnemyBehavior;
class ChargerSmallEnemyBehavior;
class ApproacherSmallEnemyBehavior;
class EliteSmallEnemyBehavior;
class SmallEnemyEnteringState;
class SmallEnemySpawnWaitState;
class SmallEnemyRangedState;
class SmallEnemyChargingState;
class SmallEnemyRushingState;
class SmallEnemyApproachingState;
class SmallEnemyNearCombatState;
class SmallEnemyEscapingState;

class SmallEnemy final : public Enemy
{
public:
	SmallEnemy();
	~SmallEnemy() override;

	SmallEnemyRole GetRole() const { return role_; }
	SmallEnemyState GetBehaviorState() const { return state_; }
	const char* GetRoleName() const override { return ToString(role_); }
	const char* GetBehaviorStateName() const override { return ToString(state_); }

protected:
	void InitializeSpecific(const EnemyCreateConfig& config) override;
	void UpdateBehavior(const float& deltaTime) override;
	void UpdateAfterTransform() override;
	void UpdateDebugUI() override;
	void OnPlayerCollision() override;
	float GetRailFollowBlend() const override;

private:
	friend class BasicSmallEnemyBehavior;
	friend class TurretSmallEnemyBehavior;
	friend class ChargerSmallEnemyBehavior;
	friend class ApproacherSmallEnemyBehavior;
	friend class EliteSmallEnemyBehavior;
	friend class SmallEnemyEnteringState;
	friend class SmallEnemySpawnWaitState;
	friend class SmallEnemyRangedState;
	friend class SmallEnemyChargingState;
	friend class SmallEnemyRushingState;
	friend class SmallEnemyApproachingState;
	friend class SmallEnemyNearCombatState;
	friend class SmallEnemyEscapingState;

	void ChangeState(SmallEnemyState state);
	SmallEnemyState GetInitialRoleState() const;
	bool UpdateForwardMovement(float deltaTime);
	void UpdateEscaping(float deltaTime);
	void UpdateShooting(float deltaTime);
	void FireProjectile();
	bool UsesBurstFire() const;
	bool CanShootInCurrentState() const;

	SmallEnemyRole role_ = SmallEnemyRole::Basic;
	SmallEnemyState state_ = SmallEnemyState::Ranged;
	EnemyCreateConfig config_{};
	std::unique_ptr<ISmallEnemyRoleBehavior> roleBehavior_;
	std::unique_ptr<ISmallEnemyState> stateBehavior_;

	// チャージなどに移行してからの時間
	float stateElapsedTime_ = 0.0f;
	// 射撃間隔タイマー
	float shotElapsedTime_ = 0.0f;
	// バースト射撃間隔タイマー
	float burstElapsedTime_ = 0.0f;
	// バースト射撃残り回数
	uint32_t burstShotsRemaining_ = 0;
	// 突撃移行後の移動距離
	float rushTravelledDistance_ = 0.0f;
	// エリートの突撃が発動したかどうか
	bool eliteChargeTriggered_ = false;
	// ダメージを受けた後に体当たりダメージを復活させるフラグ
	bool restoreBodyDamageNextUpdate_ = false;
	// チャージが完了しているかどうか
	bool chargeReady_ = false;
	// 逃げる方向（-1.0f:左, 1.0f:右）
	float escapeDirectionX_ = 1.0f;

	Vector3 stateStartPosition_{};
	Vector3 stateTargetPosition_{};
	Vector3 spawnTargetPosition_{};
	Vector3 rushDirection_{ 0.0f, 0.0f, -1.0f };
};
