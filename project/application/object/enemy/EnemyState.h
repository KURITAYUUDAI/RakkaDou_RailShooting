#pragma once
#include "myMath.h"
#include <memory>
#include "AIHandler.h"
#include "EnemyCommand.h"
#include "EnemyUtility.h"

class Enemy;

class IEnemyState
{
public:
	virtual void Initialize(Enemy* enemy) = 0;
	virtual void Update(Enemy* enemy, const float& deltaTime) = 0;
	virtual void Draw(Enemy* enemy) = 0;
	virtual void Finalize(Enemy* enemy) = 0;
};

class EnemyIdleState : public IEnemyState
{
public:
	void Initialize(Enemy* enemy) override;
	void Update(Enemy* enemy, const float& deltaTime) override;
	void Draw(Enemy* enemy) override;
	void Finalize(Enemy* enemy) override;

private:

	std::unique_ptr<IEnemyCommand> moveCommand_;
	std::unique_ptr<IEnemyCommand> shotCommand_;
	std::unique_ptr<IEnemyCommand> attackCommand_;
};

class EnemyMoveState : public IEnemyState
{
public:
	void Initialize(Enemy* enemy) override;
	void Update(Enemy* enemy, const float& deltaTime) override;
	void Draw(Enemy* enemy) override;
	void Finalize(Enemy* enemy) override;
};

class EnemyBossBattleState : public IEnemyState
{
public:
	void Initialize(Enemy* enemy) override;
	void Update(Enemy* enemy, const float& deltaTime) override;
	void Draw(Enemy* enemy) override;
	void Finalize(Enemy* enemy) override;

private:
	void ChangePhase(Enemy* enemy, EnemyBattlePhase phase);
	void RotateTowardPlayer(Enemy* enemy, float deltaTime) const;

	EnemyBattlePhase phase_ = EnemyBattlePhase::MoveToRanged;
	float timer_ = 0.0f;
	uint32_t shotCount_ = 0;
	Vector3 rangedPosition_{};
	Vector3 phaseStartPosition_{};
	Vector3 handStartPosition_{};

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
	// RailRoot原点へ到着後、Playerへ向くY回転の追従速度。
	static constexpr float kTurnSpeed_ = 8.0f;
	// 遠距離ターンではPlayerのローカルZから十分前方へ離れる。
	static constexpr float kRangedDistanceFromPlayer_ = 40.0f;
	// 近接攻撃はRailRootローカル原点を中心に行う。
	static constexpr Vector3 kMeleePosition_ = { 0.0f, 0.0f, 0.0f };
};

class EnemyShotState : public IEnemyState
{
public:
	void Initialize(Enemy* enemy) override;
	void Update(Enemy* enemy, const float& deltaTime) override;
	void Draw(Enemy* enemy) override;
	void Finalize(Enemy* enemy) override;

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

	void Initialize(Enemy* enemy) override;
	void Update(Enemy* enemy, const float& deltaTime) override;
	void Draw(Enemy* enemy) override;
	void Finalize(Enemy* enemy) override;

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
