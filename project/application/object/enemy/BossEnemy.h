#pragma once

#include "Enemy.h"
#include "EnemyState.h"
#include "AIHandler.h"

class BossEnemy final : public Enemy
{
public:
	bool GetIsBoss() const override { return true; }
	const char* GetRoleName() const override { return "Boss"; }
	const char* GetBehaviorStateName() const override { return "BossBattle"; }

	Collider* GetAttackCollider() override { return colliderAttack_.get(); }
	bool GetIsAttackColliderActive() const override { return isAttackColliderActive_; }
	EnemyBattlePhase GetBattlePhase() const override { return battlePhase_; }
	bool CanReceiveMeleeApproach() const override;

	AIHandler* GetAIHandler() { return &handler_; }
	const Vector3& GetVelocity() const { return velocity_; }
	const EulerTransform& GetRightHandTransform() const { return rightHandTransform_; }

	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	void SetRightHandScale(const Vector3& scale) { rightHandTransform_.scale = scale; }
	void SetRightHandRotate(const Vector3& rotate) { rightHandTransform_.rotate = rotate; }
	void SetRightHandTranslate(const Vector3& translate) { rightHandTransform_.translate = translate; }
	void SetRightHandTransform(const EulerTransform& transform) { rightHandTransform_ = transform; }
	void SetAttackColliderActive(bool isActive) { isAttackColliderActive_ = isActive; }
	void SetBattlePhase(EnemyBattlePhase phase) { battlePhase_ = phase; }

	void ChangeState(std::unique_ptr<IEnemyState> newState);
	void Move(float directionX, float directionY);
	void Decelerate();
	void Shot();
	void FireProjectile();
	void Attack();
	void Damage(int damage, const Vector3& hitPosition) override;

protected:
	void InitializeSpecific(const EnemyCreateConfig& config) override;
	void UpdateBehavior(const float& deltaTime) override;
	void UpdateAfterTransform() override;
	void StartDefeatedSequence() override;
	void UpdateDefeatedSequence(float deltaTime) override;
	void UpdateDebugUI() override;
	void DrawAdditional() override;
	void DrawAdditionalDebug() override;
	void OnPlayerCollision() override;
	float GetRailFollowBlend() const override;
	Vector3 GetVisualRotationOffset() const override;

private:
	enum class DefeatedPhase
	{
		Recoil,
		Spinning,
	};

	void StartDamageReaction();
	void UpdateDamageReaction(float deltaTime);
	float CalculateDamageRecoilAngle() const;

	AIHandler handler_;
	std::unique_ptr<IEnemyState> state_;

	Vector3 velocity_{};
	Vector3 maxSpeed_ = { 12.0f, 12.0f, 12.0f };
	float lerpFactor_ = 0.2f;
	float bulletSpeed_ = 15.0f;

	std::unique_ptr<Object3d> objectRightHand_;
	EulerTransform rightHandTransform_;
	std::unique_ptr<Collider> colliderAttack_;
	bool isAttackColliderActive_ = false;
	EnemyBattlePhase battlePhase_ = EnemyBattlePhase::Ranged;

	bool isDamageReactionActive_ = false;
	float damageReactionTimer_ = 0.0f;
	static constexpr float kDamageReactionAngle_ = -pi / 6.0f;
	static constexpr float kDamageReactionImpactDuration_ = 0.12f;
	static constexpr float kDamageReactionDuration_ = 1.0f;

	DefeatedPhase defeatedPhase_ = DefeatedPhase::Recoil;
	float defeatedPhaseTimer_ = 0.0f;
	Vector3 defeatedVisualRotation_{};
	float defeatedAngularSpeed_ = 0.0f;
	float defeatedRailAdvanceSpeed_ = 0.0f;
	float defeatedFogEndDistance_ = 0.0f;
	float defeatedRailStartZ_ = 0.0f;
	float defeatedRailTargetTravelDistance_ = 0.0f;
	float defeatedRailTravelDistance_ = 0.0f;
	static constexpr float kDefeatedRecoilImpactDuration_ = 0.12f;
	static constexpr float kDefeatedRecoilDuration_ = 0.5f;
	static constexpr float kDefeatedSequenceDuration_ = 2.5f;
	static constexpr float kDefeatedMaxAngularSpeed_ = pi / 1.0f;
	static constexpr float kDefeatedSpinAccelerationDuration_ = 1.0f;
	static constexpr float kDefeatedFogMargin_ = 5.0f;
	static constexpr float kDefeatedRailSearchStep_ = 1.0f;
	static constexpr float kDefeatedRailSearchLimit_ = 10000.0f;
};
