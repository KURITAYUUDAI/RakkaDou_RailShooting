#pragma once
#include "myMath.h"
#include <memory>
#include "AIHandler.h"
#include "EnemyCommand.h"
#include "EnemyUtility.h"

class BossEnemy;

class IEnemyState
{
public:
	virtual ~IEnemyState() = default;
	virtual void Initialize(BossEnemy* enemy) = 0;
	virtual void Update(BossEnemy* enemy, const float& deltaTime) = 0;
	virtual void Draw(BossEnemy* enemy) = 0;
	virtual void Finalize(BossEnemy* enemy) = 0;
};

class EnemyIdleState : public IEnemyState
{
public:
	void Initialize(BossEnemy* enemy) override;
	void Update(BossEnemy* enemy, const float& deltaTime) override;
	void Draw(BossEnemy* enemy) override;
	void Finalize(BossEnemy* enemy) override;

private:

	std::unique_ptr<IEnemyCommand> moveCommand_;
	std::unique_ptr<IEnemyCommand> shotCommand_;
	std::unique_ptr<IEnemyCommand> attackCommand_;
};

class EnemyMoveState : public IEnemyState
{
public:
	void Initialize(BossEnemy* enemy) override;
	void Update(BossEnemy* enemy, const float& deltaTime) override;
	void Draw(BossEnemy* enemy) override;
	void Finalize(BossEnemy* enemy) override;
};

class EnemyBossBattleState : public IEnemyState
{
public:
	void Initialize(BossEnemy* enemy) override;
	void Update(BossEnemy* enemy, const float& deltaTime) override;
	void Draw(BossEnemy* enemy) override;
	void Finalize(BossEnemy* enemy) override;

private:
	void ChangePhase(BossEnemy* enemy, EnemyBattlePhase phase);
	Vector3 GetHorizontalTrackingTarget(BossEnemy* enemy) const;

	EnemyBattlePhase phase_ = EnemyBattlePhase::MoveToRanged;
	float timer_ = 0.0f;
	uint32_t shotCount_ = 0;
	Vector3 stageStartPosition_{};
	Vector3 rangedPosition_{};
	Vector3 closePosition_{};
	Vector3 phaseStartPosition_{};
	Vector3 handStartPosition_{};
	Vector3 trackedPlayerPosition_{};

	static constexpr float kRangedDuration_ = 8.0f;
	static constexpr float kShotInterval_ = 1.0f;
	static constexpr float kMoveToRangedDuration_ = 0.3f;
	static constexpr float kApproachDuration_ = 0.3f;
	static constexpr float kCloseWaitDuration_ = 3.0f;
	static constexpr float kCloseAttackDuration_ = 1.0f;
	// 攻撃全体の前半を長めの振り上げ予兆にして、ジャスト回避の入力猶予を作る。
	static constexpr float kCloseAttackWindupRate_ = 0.45f;
	static constexpr float kCloseAttackHitEndRate_ = 0.75f;
	static constexpr float kRecoveryDuration_ = 3.0f;
	// 接近中は攻撃中より遅く追従させ、Playerの移動に少し遅れて左右へ動かす。
	static constexpr float kApproachHomingSpeed_ = 4.0f;
	static constexpr float kCloseAttackHomingSpeed_ = 8.0f;
	static constexpr Vector3 kRangedOffset_ = { 0.0f, 0.0f, 20.0f };
	// 2倍サイズの腕先だけがPlayer付近へ届く距離で止まり、モデルのめり込みを防ぐ。
	static constexpr Vector3 kCloseOffset_ = { 0.0f, 0.0f, 7.0f };
};

class EnemyShotState : public IEnemyState
{
public:
	void Initialize(BossEnemy* enemy) override;
	void Update(BossEnemy* enemy, const float& deltaTime) override;
	void Draw(BossEnemy* enemy) override;
	void Finalize(BossEnemy* enemy) override;

private:

	float timer_ = 0.0f;
	float duration_ = 0.1f;

	std::unique_ptr<IEnemyCommand> moveCommand_;
};

class EnemyAttackState : public IEnemyState
{
private:
	enum class AttackPhase
	{
		Approach,
		Homing,
		Lock,
		Attack,
		Away,
	};

public:

	void Initialize(BossEnemy* enemy) override;
	void Update(BossEnemy* enemy, const float& deltaTime) override;
	void Draw(BossEnemy* enemy) override;
	void Finalize(BossEnemy* enemy) override;

private:

	AttackPhase attackPhase_ = AttackPhase::Approach;

	Vector3 beforePosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 approachPosition_ = { 0.0f, 0.0f, 20.0f };
	Vector3 handPosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 homingPosition_ = { 0.0f, 0.0f, 0.0f };

	float timer_ = 0.0f;
	float duration_ = 0.5f;
	std::unique_ptr<IEnemyCommand> attackCommand_;
};
