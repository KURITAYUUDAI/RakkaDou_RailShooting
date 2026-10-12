#include "EnemyState.h"
#include "Enemy.h"
#include "EnemyCommand.h"
#include "SplineCurve.h"
#include <cmath>

void EnemyIdleState::Initialize(Enemy* enemy)
{
	moveCommand_ = std::make_unique<EnemyMoveCommand>(
		enemy->GetAIHandler());
	shotCommand_ = std::make_unique<EnemyShotCommand>(
		enemy->GetAIHandler());
	attackCommand_ = std::make_unique<EnemyAttackCommand>(
		enemy->GetAIHandler());
}

void EnemyIdleState::Update(Enemy* enemy, const float& deltaTime)
{
	AIHandler* handler = enemy->GetAIHandler();

	moveCommand_->Execute(enemy);
	
	if (handler->IsAttack(deltaTime))
	{
		attackCommand_->Execute(enemy);
	}
	else if (handler->IsShot(deltaTime))
	{
		shotCommand_->Execute(enemy);
	}
}

void EnemyIdleState::Draw(Enemy * enemy)
{
	(void)enemy;
}

void EnemyIdleState::Finalize(Enemy * enemy)
{
	(void)enemy;
}


void EnemyMoveState::Initialize(Enemy* enemy)
{
	(void)enemy;
}

void EnemyMoveState::Update(Enemy * enemy, const float& deltaTime)
{
	(void)enemy;
}

void EnemyMoveState::Draw(Enemy * enemy)
{
	(void)enemy;
}

void EnemyMoveState::Finalize(Enemy * enemy)
{
	(void)enemy;
}

void EnemyBossBattleState::Initialize(Enemy* enemy)
{
	phase_ = EnemyBattlePhase::MoveToRanged;
	timer_ = 0.0f;
	shotCount_ = 0;
	rangedPosition_ = enemy->GetTranslate();
	phaseStartPosition_ = enemy->GetTranslate();
	handStartPosition_ = enemy->GetRightHandTransform().translate;
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
	enemy->SetAttackColliderActive(false);
	enemy->SetBattlePhase(phase_);
}

void EnemyBossBattleState::Update(Enemy* enemy, const float& deltaTime)
{
	timer_ += deltaTime;

	switch (phase_)
	{
	case EnemyBattlePhase::MoveToRanged:
		// Playerと同じRailRootローカル座標で、行動範囲より前方の位置を作る。
		rangedPosition_.z =
			enemy->GetPlayerLocalPosition().z + kRangedDistanceFromPlayer_;
		enemy->SetTranslate(Lerp(
			phaseStartPosition_, rangedPosition_,
			std::min(timer_ / kMoveToRangedDuration_, 1.0f)));
		if (timer_ >= kMoveToRangedDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::Ranged);
		}
		break;

	case EnemyBattlePhase::Ranged:
		enemy->SetTranslate(rangedPosition_);
		while (shotCount_ < 4 && timer_ >= (shotCount_ + 1) * kShotInterval_)
		{
			enemy->FireProjectile();
			++shotCount_;
		}
		if (timer_ >= kRangedDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::Approach);
		}
		break;

	case EnemyBattlePhase::Approach:
		// Playerの位置には寄らず、RailRootローカル原点へ移動する。
		enemy->SetTranslate(Lerp(
			phaseStartPosition_, kMeleePosition_,
			std::min(timer_ / kApproachDuration_, 1.0f)));
		if (timer_ >= kApproachDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::CloseWait);
		}
		break;

	case EnemyBattlePhase::CloseWait:
		// 原点に固定したまま、前方がPlayerへ向くようY軸回転だけを追従する。
		enemy->SetTranslate(kMeleePosition_);
		RotateTowardPlayer(enemy, deltaTime);
		if (timer_ >= kCloseWaitDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::CloseAttack);
		}
		break;

	case EnemyBattlePhase::CloseAttack:
	{
		// 近距離攻撃中も原点から動かず、Playerへの向きだけを追従する。
		enemy->SetTranslate(kMeleePosition_);
		RotateTowardPlayer(enemy, deltaTime);
		const float attackT = std::min(timer_ / kCloseAttackDuration_, 1.0f);
		if (attackT < kCloseAttackWindupRate_)
		{
			enemy->SetRightHandTranslate(Lerp(
				handStartPosition_, { 0.0f, 5.0f, 0.0f },
				attackT / kCloseAttackWindupRate_));
		}
		else if (attackT < kCloseAttackHitEndRate_)
		{
			enemy->SetAttackColliderActive(true);
			enemy->SetRightHandTranslate(Lerp(
				{ 0.0f, 5.0f, 0.0f }, { 0.0f, -5.0f, 0.0f },
				(attackT - kCloseAttackWindupRate_) /
				(kCloseAttackHitEndRate_ - kCloseAttackWindupRate_)));
		}
		else
		{
			enemy->SetAttackColliderActive(false);
			enemy->SetRightHandTranslate(Lerp(
				{ 0.0f, -5.0f, 0.0f }, handStartPosition_,
				(attackT - kCloseAttackHitEndRate_) /
				(1.0f - kCloseAttackHitEndRate_)));
		}
		if (timer_ >= kCloseAttackDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::Recovery);
		}
		break;
	}

	case EnemyBattlePhase::Recovery:
		// 硬直中も原点を維持し、Playerへの向きを追従する。
		enemy->SetTranslate(kMeleePosition_);
		RotateTowardPlayer(enemy, deltaTime);
		if (timer_ >= kRecoveryDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::MoveToRanged);
		}
		break;
	}
}

void EnemyBossBattleState::Draw(Enemy* enemy)
{
	(void)enemy;
}

void EnemyBossBattleState::Finalize(Enemy* enemy)
{
	enemy->SetAttackColliderActive(false);
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void EnemyBossBattleState::ChangePhase(Enemy* enemy, EnemyBattlePhase phase)
{
	phase_ = phase;
	timer_ = 0.0f;
	enemy->SetBattlePhase(phase_);
	enemy->SetAttackColliderActive(false);

	if (phase_ == EnemyBattlePhase::Approach)
	{
		phaseStartPosition_ = enemy->GetTranslate();
	}
	if (phase_ == EnemyBattlePhase::CloseWait)
	{
		enemy->SetTranslate(kMeleePosition_);
	}
	if (phase_ == EnemyBattlePhase::MoveToRanged)
	{
		phaseStartPosition_ = enemy->GetTranslate();
	}
	if (phase_ == EnemyBattlePhase::CloseAttack)
	{
		handStartPosition_ = enemy->GetRightHandTransform().translate;
	}
	if (phase_ == EnemyBattlePhase::Ranged)
	{
		shotCount_ = 0;
		enemy->SetRightHandTranslate(handStartPosition_);
	}
}

void EnemyBossBattleState::RotateTowardPlayer(
	Enemy* enemy, float deltaTime) const
{
	const Vector3 toPlayer =
		enemy->GetPlayerLocalPosition() - enemy->GetTranslate();
	if (toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z <= 0.000001f)
	{
		return;
	}

	// Enemyの攻撃方向はローカル-Zなので、その軸をPlayerへ向ける。
	const float targetYaw = std::atan2(-toPlayer.x, -toPlayer.z);
	Vector3 rotation = enemy->GetRotate();
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kTwoPi = kPi * 2.0f;
	float yawDelta = std::fmod(targetYaw - rotation.y + kPi, kTwoPi);
	if (yawDelta < 0.0f)
	{
		yawDelta += kTwoPi;
	}
	yawDelta -= kPi;
	rotation.y += yawDelta * std::min(deltaTime * kTurnSpeed_, 1.0f);
	enemy->SetRotate(rotation);
}



void EnemyShotState::Initialize(Enemy* enemy)
{
	moveCommand_ = std::make_unique<EnemyMoveCommand>(
		enemy->GetAIHandler());
}

void EnemyShotState::Update(Enemy * enemy, const float& deltaTime)
{
	moveCommand_->Execute(enemy);

	timer_ += deltaTime;
	if (timer_ >= duration_)
	{
		enemy->ChangeState(std::make_unique<EnemyIdleState>());
	}
}

void EnemyShotState::Draw(Enemy * enemy)
{
	(void)enemy;
}

void EnemyShotState::Finalize(Enemy * enemy)
{
	(void)enemy;
}

void EnemyAttackState::Initialize(Enemy* enemy)
{
	attackPhase_ = AttackPhase::Approach;
	timer_ = 0.0f;
	duration_ = 0.5f;
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
	enemy->SetAttackColliderActive(false);

	// 動的に再中心化されても壊れないよう、Playerからの相対位置で軌道を持つ。
	beforePosition_ = enemy->GetTranslate() - enemy->GetPlayerLocalPosition();
	approachPosition_ = { 0.0f, 0.0f, 5.0f };
}

void EnemyAttackState::Update(Enemy * enemy, const float& deltaTime)
{
	timer_ += deltaTime;
	switch (attackPhase_)
	{
	case EnemyAttackState::AttackPhase::Approach:
		enemy->SetTranslate(enemy->GetPlayerLocalPosition() +
			Lerp(beforePosition_, approachPosition_, std::min(timer_ / duration_, 1.0f)));

		if (timer_ >= duration_)
		{
			attackPhase_ = AttackPhase::Homing;
			timer_ = 0.0f;
			duration_ = 0.5f;
			handPosition_ = enemy->GetRightHandTransform().translate;
		}
		break;

	case EnemyAttackState::AttackPhase::Homing:
		enemy->SetTranslate(enemy->GetPlayerLocalPosition() + approachPosition_);
		enemy->SetRightHandTranslate(Lerp(
			handPosition_, {0.0f, 5.0f, 0.0f}, std::min(timer_ / duration_, 1.0f)));

		if (timer_ >= duration_)
		{
			attackPhase_ = AttackPhase::Attack;
			timer_ = 0.0f;
			duration_ = 0.4f;
			handPosition_ = enemy->GetRightHandTransform().translate;
			enemy->SetAttackColliderActive(true);
		}
		break;

	case EnemyAttackState::AttackPhase::Attack:

		enemy->SetTranslate(enemy->GetPlayerLocalPosition() + approachPosition_);
		enemy->SetRightHandTranslate(Lerp(
			handPosition_, {0.0f, -5.0f, 0.0f}, std::min(timer_ / duration_, 1.0f)));

		if (timer_ >= duration_)
		{
			attackPhase_ = AttackPhase::Away;
			enemy->SetAttackColliderActive(false);
			timer_ = 0.0f;
			duration_ = 1.0f;
			handPosition_ = enemy->GetRightHandTransform().translate;
		}
		break;

	case EnemyAttackState::AttackPhase::Away:

		enemy->SetTranslate(enemy->GetPlayerLocalPosition() +
			Lerp(approachPosition_, beforePosition_, std::min(timer_ / duration_, 1.0f)));
		enemy->SetRightHandTranslate(Lerp(
			handPosition_, { -2.5f, 0.0f, 0.0f }, std::min(timer_ / duration_, 1.0f)));

		if (timer_ >= duration_)
		{
			enemy->ChangeState(std::make_unique<EnemyIdleState>());
		}
		break;

	default:
		break;
	}
}

void EnemyAttackState::Draw(Enemy * enemy)
{
	(void)enemy;
}

void EnemyAttackState::Finalize(Enemy * enemy)
{
	enemy->SetAttackColliderActive(false);
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
}
