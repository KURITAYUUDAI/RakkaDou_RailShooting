#include "EnemyState.h"
#include "BossEnemy.h"
#include "EnemyCommand.h"
#include "SplineCurve.h"

#include <optional>

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

class IEnemyBossBattlePhase
{
public:
	virtual ~IEnemyBossBattlePhase() = default;
	virtual void Enter(EnemyBossBattleState&, BossEnemy*) {}
	virtual std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state,
		BossEnemy* enemy,
		float deltaTime) = 0;
};

class EnemyMoveToRangedPhase final : public IEnemyBossBattlePhase
{
public:
	void Enter(EnemyBossBattleState& state, BossEnemy* enemy) override
	{
		state.phaseStartPosition_ = enemy->GetTranslate();
	}

	std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state, BossEnemy* enemy, float) override
	{
		enemy->SetTranslate(Lerp(
			state.phaseStartPosition_, state.rangedPosition_,
			std::min(
				state.timer_ / EnemyBossBattleState::kMoveToRangedDuration_,
				1.0f)));
		if (state.timer_ >= EnemyBossBattleState::kMoveToRangedDuration_)
		{
			return EnemyBattlePhase::Ranged;
		}
		return std::nullopt;
	}
};

class EnemyRangedPhase final : public IEnemyBossBattlePhase
{
public:
	void Enter(EnemyBossBattleState& state, BossEnemy* enemy) override
	{
		state.shotCount_ = 0;
		enemy->SetRightHandTranslate(state.handStartPosition_);
	}

	std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state, BossEnemy* enemy, float) override
	{
		enemy->SetTranslate(state.rangedPosition_);
		while (state.shotCount_ < 4 &&
			state.timer_ >=
				(state.shotCount_ + 1) * EnemyBossBattleState::kShotInterval_)
		{
			enemy->FireProjectile();
			++state.shotCount_;
		}
		if (state.timer_ >= EnemyBossBattleState::kRangedDuration_)
		{
			return EnemyBattlePhase::Approach;
		}
		return std::nullopt;
	}
};

class EnemyApproachPhase final : public IEnemyBossBattlePhase
{
public:
	void Enter(EnemyBossBattleState& state, BossEnemy* enemy) override
	{
		state.phaseStartPosition_ = enemy->GetTranslate();
		state.trackedPlayerPosition_ =
			state.GetHorizontalTrackingTarget(enemy);
		state.closePosition_ =
			state.trackedPlayerPosition_ + EnemyBossBattleState::kCloseOffset_;
	}

	std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state,
		BossEnemy* enemy,
		float deltaTime) override
	{
		state.trackedPlayerPosition_ = Lerp(
			state.trackedPlayerPosition_,
			state.GetHorizontalTrackingTarget(enemy),
			std::min(
				deltaTime * EnemyBossBattleState::kApproachHomingSpeed_, 1.0f));
		state.closePosition_ =
			state.trackedPlayerPosition_ + EnemyBossBattleState::kCloseOffset_;
		enemy->SetTranslate(Lerp(
			state.phaseStartPosition_, state.closePosition_,
			std::min(
				state.timer_ / EnemyBossBattleState::kApproachDuration_, 1.0f)));
		if (state.timer_ >= EnemyBossBattleState::kApproachDuration_)
		{
			return EnemyBattlePhase::CloseWait;
		}
		return std::nullopt;
	}
};

class EnemyCloseWaitPhase final : public IEnemyBossBattlePhase
{
public:
	std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state,
		BossEnemy* enemy,
		float deltaTime) override
	{
		enemy->SetAttackChargeProgress(std::min(
			state.timer_ / EnemyBossBattleState::kCloseWaitDuration_, 1.0f));
		state.trackedPlayerPosition_ = Lerp(
			state.trackedPlayerPosition_,
			state.GetHorizontalTrackingTarget(enemy),
			std::min(
				deltaTime * EnemyBossBattleState::kApproachHomingSpeed_, 1.0f));
		state.closePosition_ =
			state.trackedPlayerPosition_ + EnemyBossBattleState::kCloseOffset_;
		enemy->SetTranslate(state.closePosition_);
		if (state.timer_ >= EnemyBossBattleState::kCloseWaitDuration_)
		{
			return EnemyBattlePhase::CloseAttack;
		}
		return std::nullopt;
	}
};

class EnemyCloseAttackPhase final : public IEnemyBossBattlePhase
{
public:
	void Enter(EnemyBossBattleState& state, BossEnemy* enemy) override
	{
		state.handStartPosition_ = enemy->GetRightHandTransform().translate;
		enemy->SetAttackChargeProgress(1.0f);
	}

	std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state,
		BossEnemy* enemy,
		float deltaTime) override
	{
		const Vector3 homingTarget =
			state.GetHorizontalTrackingTarget(enemy) +
			EnemyBossBattleState::kCloseOffset_;
		state.closePosition_ = Lerp(
			state.closePosition_, homingTarget,
			std::min(
				deltaTime * EnemyBossBattleState::kCloseAttackHomingSpeed_,
				1.0f));
		enemy->SetTranslate(state.closePosition_);
		const float attackT = std::min(
			state.timer_ / EnemyBossBattleState::kCloseAttackDuration_, 1.0f);
		if (attackT < EnemyBossBattleState::kCloseAttackWindupRate_)
		{
			enemy->SetRightHandTranslate(Lerp(
				state.handStartPosition_, { 0.0f, 5.0f, 0.0f },
				attackT / EnemyBossBattleState::kCloseAttackWindupRate_));
		}
		else if (attackT < EnemyBossBattleState::kCloseAttackHitEndRate_)
		{
			enemy->SetAttackColliderActive(true);
			enemy->SetRightHandTranslate(Lerp(
				{ 0.0f, 5.0f, 0.0f }, { 0.0f, -5.0f, 0.0f },
				(attackT - EnemyBossBattleState::kCloseAttackWindupRate_) /
				(EnemyBossBattleState::kCloseAttackHitEndRate_ -
					EnemyBossBattleState::kCloseAttackWindupRate_)));
		}
		else
		{
			enemy->SetAttackColliderActive(false);
			enemy->SetRightHandTranslate(Lerp(
				{ 0.0f, -5.0f, 0.0f }, state.handStartPosition_,
				(attackT - EnemyBossBattleState::kCloseAttackHitEndRate_) /
				(1.0f - EnemyBossBattleState::kCloseAttackHitEndRate_)));
		}
		if (state.timer_ >= EnemyBossBattleState::kCloseAttackDuration_)
		{
			return EnemyBattlePhase::Recovery;
		}
		return std::nullopt;
	}
};

class EnemyRecoveryPhase final : public IEnemyBossBattlePhase
{
public:
	std::optional<EnemyBattlePhase> Update(
		EnemyBossBattleState& state, BossEnemy* enemy, float) override
	{
		enemy->SetTranslate(state.closePosition_);
		if (state.timer_ >= EnemyBossBattleState::kRecoveryDuration_)
		{
			return EnemyBattlePhase::MoveToRanged;
		}
		return std::nullopt;
	}
};

namespace
{
	std::unique_ptr<IEnemyBossBattlePhase> CreateBossBattlePhase(
		EnemyBattlePhase phase)
	{
		switch (phase)
		{
		case EnemyBattlePhase::MoveToRanged:
			return std::make_unique<EnemyMoveToRangedPhase>();
		case EnemyBattlePhase::Ranged:
			return std::make_unique<EnemyRangedPhase>();
		case EnemyBattlePhase::Approach:
			return std::make_unique<EnemyApproachPhase>();
		case EnemyBattlePhase::CloseWait:
			return std::make_unique<EnemyCloseWaitPhase>();
		case EnemyBattlePhase::CloseAttack:
			return std::make_unique<EnemyCloseAttackPhase>();
		case EnemyBattlePhase::Recovery:
			return std::make_unique<EnemyRecoveryPhase>();
		}
		return nullptr;
	}
}

EnemyBossBattleState::EnemyBossBattleState() = default;
EnemyBossBattleState::~EnemyBossBattleState() = default;

void EnemyBossBattleState::Initialize(BossEnemy* enemy)
{
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
	ChangePhase(enemy, EnemyBattlePhase::MoveToRanged);
}

void EnemyBossBattleState::Update(BossEnemy* enemy, const float& deltaTime)
{
	timer_ += deltaTime;
	if (!phaseBehavior_)
	{
		return;
	}

	const std::optional<EnemyBattlePhase> nextPhase =
		phaseBehavior_->Update(*this, enemy, deltaTime);
	if (nextPhase)
	{
		ChangePhase(enemy, *nextPhase);
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
	phaseBehavior_ = CreateBossBattlePhase(phase);
	timer_ = 0.0f;
	enemy->SetBattlePhase(phase_);
	enemy->SetAttackColliderActive(false);
	enemy->SetAttackChargeProgress(0.0f);
	if (phaseBehavior_)
	{
		phaseBehavior_->Enter(*this, enemy);
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

class IEnemyAttackPhase
{
public:
	struct Result
	{
		std::optional<EnemyAttackState::AttackPhase> nextPhase;
		bool complete = false;
	};

	virtual ~IEnemyAttackPhase() = default;
	virtual void Enter(EnemyAttackState&, BossEnemy*) {}
	virtual Result Update(
		EnemyAttackState& state, BossEnemy* enemy, float deltaTime) = 0;
};

class EnemyAttackApproachPhase final : public IEnemyAttackPhase
{
public:
	void Enter(EnemyAttackState& state, BossEnemy* enemy) override
	{
		state.duration_ = 0.5f;
		enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
		enemy->SetAttackColliderActive(false);
		enemy->SetAttackChargeProgress(0.0f);
		state.beforePosition_ =
			enemy->GetTranslate() - enemy->GetPlayerLocalPosition();
		state.approachPosition_ = { 0.0f, 0.0f, 5.0f };
	}

	Result Update(EnemyAttackState& state, BossEnemy* enemy, float) override
	{
		enemy->SetTranslate(enemy->GetPlayerLocalPosition() +
			Lerp(
				state.beforePosition_, state.approachPosition_,
				std::min(state.timer_ / state.duration_, 1.0f)));
		if (state.timer_ >= state.duration_)
		{
			return { EnemyAttackState::AttackPhase::Homing, false };
		}
		return {};
	}
};

class EnemyAttackHomingPhase final : public IEnemyAttackPhase
{
public:
	void Enter(EnemyAttackState& state, BossEnemy* enemy) override
	{
		state.duration_ = 0.5f;
		state.handPosition_ = enemy->GetRightHandTransform().translate;
	}

	Result Update(EnemyAttackState& state, BossEnemy* enemy, float) override
	{
		enemy->SetAttackChargeProgress(
			std::min(state.timer_ / state.duration_, 1.0f));
		enemy->SetTranslate(
			enemy->GetPlayerLocalPosition() + state.approachPosition_);
		enemy->SetRightHandTranslate(Lerp(
			state.handPosition_, { 0.0f, 5.0f, 0.0f },
			std::min(state.timer_ / state.duration_, 1.0f)));
		if (state.timer_ >= state.duration_)
		{
			return { EnemyAttackState::AttackPhase::Attack, false };
		}
		return {};
	}
};

class EnemyAttackStrikePhase final : public IEnemyAttackPhase
{
public:
	void Enter(EnemyAttackState& state, BossEnemy* enemy) override
	{
		state.duration_ = 0.4f;
		state.handPosition_ = enemy->GetRightHandTransform().translate;
		enemy->SetAttackChargeProgress(1.0f);
		enemy->SetAttackColliderActive(true);
	}

	Result Update(EnemyAttackState& state, BossEnemy* enemy, float) override
	{
		enemy->SetTranslate(
			enemy->GetPlayerLocalPosition() + state.approachPosition_);
		enemy->SetRightHandTranslate(Lerp(
			state.handPosition_, { 0.0f, -5.0f, 0.0f },
			std::min(state.timer_ / state.duration_, 1.0f)));
		if (state.timer_ >= state.duration_)
		{
			return { EnemyAttackState::AttackPhase::Away, false };
		}
		return {};
	}
};

class EnemyAttackAwayPhase final : public IEnemyAttackPhase
{
public:
	void Enter(EnemyAttackState& state, BossEnemy* enemy) override
	{
		state.duration_ = 1.0f;
		state.handPosition_ = enemy->GetRightHandTransform().translate;
		enemy->SetAttackColliderActive(false);
		enemy->SetAttackChargeProgress(0.0f);
	}

	Result Update(EnemyAttackState& state, BossEnemy* enemy, float) override
	{
		enemy->SetTranslate(enemy->GetPlayerLocalPosition() +
			Lerp(
				state.approachPosition_, state.beforePosition_,
				std::min(state.timer_ / state.duration_, 1.0f)));
		enemy->SetRightHandTranslate(Lerp(
			state.handPosition_, { -2.5f, 0.0f, 0.0f },
			std::min(state.timer_ / state.duration_, 1.0f)));
		return { std::nullopt, state.timer_ >= state.duration_ };
	}
};

EnemyAttackState::EnemyAttackState() = default;
EnemyAttackState::~EnemyAttackState() = default;

void EnemyAttackState::Initialize(BossEnemy* enemy)
{
	ChangePhase(enemy, AttackPhase::Approach);
}

void EnemyAttackState::Update(BossEnemy* enemy, const float& deltaTime)
{
	timer_ += deltaTime;
	if (!phaseBehavior_)
	{
		return;
	}

	const IEnemyAttackPhase::Result result =
		phaseBehavior_->Update(*this, enemy, deltaTime);
	if (result.complete)
	{
		enemy->ChangeState(std::make_unique<EnemyIdleState>());
		return;
	}
	if (result.nextPhase)
	{
		ChangePhase(enemy, *result.nextPhase);
	}
}

void EnemyAttackState::ChangePhase(BossEnemy* enemy, AttackPhase phase)
{
	timer_ = 0.0f;
	switch (phase)
	{
	case AttackPhase::Approach:
		phaseBehavior_ = std::make_unique<EnemyAttackApproachPhase>();
		break;
	case AttackPhase::Homing:
		phaseBehavior_ = std::make_unique<EnemyAttackHomingPhase>();
		break;
	case AttackPhase::Attack:
		phaseBehavior_ = std::make_unique<EnemyAttackStrikePhase>();
		break;
	case AttackPhase::Away:
		phaseBehavior_ = std::make_unique<EnemyAttackAwayPhase>();
		break;
	}
	phaseBehavior_->Enter(*this, enemy);
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
