#include "EnemyState.h"
#include "BossEnemy.h"
#include "EnemyCommand.h"
#include "SplineCurve.h"

void EnemyIdleState::Initialize(BossEnemy* enemy)
{
	moveCommand_ = std::make_unique<EnemyMoveCommand>(
		enemy->GetAIHandler());
	shotCommand_ = std::make_unique<EnemyShotCommand>(
		enemy->GetAIHandler());
	attackCommand_ = std::make_unique<EnemyAttackCommand>(
		enemy->GetAIHandler());
}

void EnemyIdleState::Update(BossEnemy* enemy, const float& deltaTime)
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

void EnemyIdleState::Draw(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyIdleState::Finalize(BossEnemy* enemy)
{
	(void)enemy;
}


void EnemyMoveState::Initialize(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyMoveState::Update(BossEnemy* enemy, const float& deltaTime)
{
	(void)enemy;
}

void EnemyMoveState::Draw(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyMoveState::Finalize(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyBossBattleState::Initialize(BossEnemy* enemy)
{
	phase_ = EnemyBattlePhase::MoveToRanged;
	timer_ = 0.0f;
	shotCount_ = 0;
	stageStartPosition_ = enemy->GetTranslate();
	rangedPosition_ = stageStartPosition_ + kRangedOffset_;
	closePosition_ = stageStartPosition_;
	phaseStartPosition_ = stageStartPosition_;
	trackedPlayerPosition_ = GetHorizontalTrackingTarget(enemy);
	handStartPosition_ = enemy->GetRightHandTransform().translate;
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
	enemy->SetAttackColliderActive(false);
	enemy->SetAttackChargeProgress(0.0f);
	enemy->SetBattlePhase(phase_);
}

void EnemyBossBattleState::Update(BossEnemy* enemy, const float& deltaTime)
{
	timer_ += deltaTime;

	switch (phase_)
	{
	case EnemyBattlePhase::MoveToRanged:
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
		// 奥へ接近しつつ、PlayerのX座標だけを少し遅れて追従する。
		trackedPlayerPosition_ = Lerp(
			trackedPlayerPosition_, GetHorizontalTrackingTarget(enemy),
			std::min(deltaTime * kApproachHomingSpeed_, 1.0f));
		closePosition_ = trackedPlayerPosition_ + kCloseOffset_;
		enemy->SetTranslate(Lerp(
			phaseStartPosition_, closePosition_,
			std::min(timer_ / kApproachDuration_, 1.0f)));
		if (timer_ >= kApproachDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::CloseWait);
		}
		break;

	case EnemyBattlePhase::CloseWait:
		// 攻撃前の隙も間合いから外れないよう、接近時と同じ遅さで追従を続ける。
		enemy->SetAttackChargeProgress(std::min(
			timer_ / kCloseWaitDuration_, 1.0f));
		trackedPlayerPosition_ = Lerp(
			trackedPlayerPosition_, GetHorizontalTrackingTarget(enemy),
			std::min(deltaTime * kApproachHomingSpeed_, 1.0f));
		closePosition_ = trackedPlayerPosition_ + kCloseOffset_;
		enemy->SetTranslate(closePosition_);
		if (timer_ >= kCloseWaitDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::CloseAttack);
		}
		break;

	case EnemyBattlePhase::CloseAttack:
	{
		// 近距離攻撃中もPlayerのX座標だけを追従する。
		// 共通のbattleWorldTransform_上のローカル座標同士で計算する。
		const Vector3 homingTarget =
			GetHorizontalTrackingTarget(enemy) + kCloseOffset_;
		closePosition_ = Lerp(
			closePosition_, homingTarget,
			std::min(deltaTime * kCloseAttackHomingSpeed_, 1.0f));
		enemy->SetTranslate(closePosition_);
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
		// 近接位置を次の遠距離攻撃前のアイドリング位置として使う。
		enemy->SetTranslate(closePosition_);
		if (timer_ >= kRecoveryDuration_)
		{
			ChangePhase(enemy, EnemyBattlePhase::MoveToRanged);
		}
		break;
	}
}

void EnemyBossBattleState::Draw(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyBossBattleState::Finalize(BossEnemy* enemy)
{
	enemy->SetAttackColliderActive(false);
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
	enemy->SetAttackChargeProgress(0.0f);
}

void EnemyBossBattleState::ChangePhase(BossEnemy* enemy, EnemyBattlePhase phase)
{
	phase_ = phase;
	timer_ = 0.0f;
	enemy->SetBattlePhase(phase_);
	enemy->SetAttackColliderActive(false);
	enemy->SetAttackChargeProgress(
		phase_ == EnemyBattlePhase::CloseAttack ? 1.0f : 0.0f);

	if (phase_ == EnemyBattlePhase::Approach)
	{
		phaseStartPosition_ = enemy->GetTranslate();
		trackedPlayerPosition_ = GetHorizontalTrackingTarget(enemy);
		closePosition_ = trackedPlayerPosition_ + kCloseOffset_;
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

Vector3 EnemyBossBattleState::GetHorizontalTrackingTarget(BossEnemy* enemy) const
{
	Vector3 target = enemy->GetPlayerLocalPosition();
	// 追尾で上下には動かさず、Enemyの初期Y座標を維持する。
	target.y = stageStartPosition_.y;
	return target;
}



void EnemyShotState::Initialize(BossEnemy* enemy)
{
	moveCommand_ = std::make_unique<EnemyMoveCommand>(
		enemy->GetAIHandler());
}

void EnemyShotState::Update(BossEnemy* enemy, const float& deltaTime)
{
	moveCommand_->Execute(enemy);

	timer_ += deltaTime;
	if (timer_ >= duration_)
	{
		enemy->ChangeState(std::make_unique<EnemyIdleState>());
	}
}

void EnemyShotState::Draw(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyShotState::Finalize(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyAttackState::Initialize(BossEnemy* enemy)
{
	attackPhase_ = AttackPhase::Approach;
	timer_ = 0.0f;
	duration_ = 0.5f;
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
	enemy->SetAttackColliderActive(false);
	enemy->SetAttackChargeProgress(0.0f);

	// 動的に再中心化されても壊れないよう、Playerからの相対位置で軌道を持つ。
	beforePosition_ = enemy->GetTranslate() - enemy->GetPlayerLocalPosition();
	approachPosition_ = { 0.0f, 0.0f, 5.0f };
}

void EnemyAttackState::Update(BossEnemy* enemy, const float& deltaTime)
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
		enemy->SetAttackChargeProgress(
			std::min(timer_ / duration_, 1.0f));
		enemy->SetTranslate(enemy->GetPlayerLocalPosition() + approachPosition_);
		enemy->SetRightHandTranslate(Lerp(
			handPosition_, {0.0f, 5.0f, 0.0f}, std::min(timer_ / duration_, 1.0f)));

		if (timer_ >= duration_)
		{
			attackPhase_ = AttackPhase::Attack;
			enemy->SetAttackChargeProgress(1.0f);
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
		enemy->SetAttackChargeProgress(0.0f);

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

void EnemyAttackState::Draw(BossEnemy* enemy)
{
	(void)enemy;
}

void EnemyAttackState::Finalize(BossEnemy* enemy)
{
	enemy->SetAttackColliderActive(false);
	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
	enemy->SetAttackChargeProgress(0.0f);
}
