#pragma once

#include "Enemy.h"

class SmallEnemy final : public Enemy
{
public:
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

private:
	void ChangeState(SmallEnemyState state);
	void BeginRoleBehavior();
	void UpdateSpawnEntry(float deltaTime);
	void UpdateForwardMovement(float deltaTime);
	void UpdateCharger(float deltaTime);
	void UpdateApproacher(float deltaTime);
	void UpdateElite(float deltaTime);
	void UpdateEscaping(float deltaTime);
	void UpdateShooting(float deltaTime);
	void FireProjectile();
	bool UsesBurstFire() const;
	bool CanShootInCurrentState() const;

	SmallEnemyRole role_ = SmallEnemyRole::Basic;
	SmallEnemyState state_ = SmallEnemyState::Ranged;
	EnemyCreateConfig config_{};

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
