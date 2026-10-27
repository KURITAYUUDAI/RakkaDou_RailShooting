#define NOMINMAX
#include "PlayerState.h"
#include "Player.h"

void PlayerIdleState::Initialize(Player* player)
{
	moveCommand_ = std::make_unique<MoveHorizontalCommand>(
		player->GetInputHandler());
	shotCommand_ = std::make_unique<LongPressShotCommand>(
		player->GetInputHandler());
	avoidCommand_ = std::make_unique<AvoidCommand>(
		player->GetInputHandler());
	meleeAttackCommand_ = std::make_unique<MeleeAttackCommand>(
		player->GetInputHandler());
}

void PlayerIdleState::Update(Player * player, const float& deltaTime)
{
	IInputHandler* handler = player->GetInputHandler();

	moveCommand_->Execute(player);
	
	if (handler->IsActionTriggered(InputAction::Shot))
	{
		shotCommand_->Update(player, deltaTime);
		return; // ★ 追加: Shot()内部でChangeStateされる
	}
	if (handler->IsActionTriggered(InputAction::Avoid))
	{
		shotCommand_->Cancel(player);
		avoidCommand_->Execute(player);
		return; // ★ 追加: Avoid()内部でChangeStateされる
	}
	if (handler->IsActionTriggered(InputAction::Melee))
	{
		shotCommand_->Cancel(player);
		meleeAttackCommand_->Execute(player);
		return;
	}
	const LongPressShotCommand::Result shotResult = shotCommand_->Update(player, deltaTime);
	if (shotResult == LongPressShotCommand::Result::ChargedShot)
	{
		player->ChargedShot();
		return;
	}
	if (shotResult == LongPressShotCommand::Result::NormalShot)
	{
		player->Shot();
		return;
	}
}

void PlayerIdleState::Draw(Player* player)
{
	(void)player;
}

void PlayerIdleState::Finalize(Player* player)
{
	shotCommand_->Cancel(player);
}

void PlayerShotState::Initialize(Player* player)
{
	moveCommand_ = std::make_unique<MoveHorizontalCommand>(
		player->GetInputHandler());
}

void PlayerShotState::Update(Player* player, const float& deltaTime)
{
	moveCommand_->Execute(player);

	timer_ += deltaTime;
	if (timer_ >= duration_)
	{
		/*player->ClearLockOn();*/
		player->ChangeState(std::make_unique<PlayerIdleState>());
	}
}

void PlayerShotState::Draw(Player* player)
{
	(void)player;
}

void PlayerShotState::Finalize(Player* player)
{
	(void)player;
}

void PlayerAvoidState::Initialize(Player* player)
{
	if (inputDirection_.x != 0.0f && inputDirection_.y != 0.0f)
	{
		Vector3 currentVelocity = player->GetVelocity();
		if (currentVelocity.x != 0.0f || currentVelocity.y != 0.0f)
		{
			// 入力方向に回避速度を加算
			avoidDirection_ = currentVelocity;
		}
		else
		{
			avoidDirection_ = {1.0f, 0.0f, 0.0f};
		}
	}
	else
	{
		avoidDirection_ = { inputDirection_.x, inputDirection_.y, 0.0f };
	}

	if (inputDirection_.x != 0.0f || inputDirection_.y != 0.0f)
	{
		avoidDirection_ = Normalize(avoidDirection_);
	}
	else
	{
		avoidDirection_ = Normalize(Vector3{1.0f, 0.0f, 0.0f});
	}

	//float maxAvoidRoll = 2.0f * pi; // 最大で傾ける角度（ラジアン。0.5f は約30度）

	//if (avoidDirection_.x < 0.0f) 
	//{
	//	player->SetTargetRoll({ 0.0f, 0.0f, maxAvoidRoll});  // 左回避時のロール
	//} 
	//else // if (avoidDirection_.x > 0.0f) 
	//{
	//	player->SetTargetRoll({0.0f, 0.0f, -maxAvoidRoll}); // 右回避時のロール
	//}
	
	player->SetIsHit(false);

	player->SetAvoidDirection(avoidDirection_);
	player->SetJustAvoidAccept(true);
}

void PlayerAvoidState::Update(Player* player, const float& deltaTime)
{
	timer_ += deltaTime;

	float progress = timer_ / duration_;
	if (progress > 1.0f) progress = 1.0f;

	float oneRotation = 2.0f * pi;
	float currentRoll = 0.0f;

	if (avoidDirection_.x < 0.0f) 
	{
		currentRoll = progress * oneRotation; // 左回転
	}
	else 
	{
		currentRoll = -progress * oneRotation;  // 右回転
	}

	Vector3 currentRotate = player->GetRotate();
	currentRotate.z = currentRoll;
	player->SetRotate(currentRotate);

	float speedRate = 1.0f - (progress * progress);

	float currentSpeed = avoidSpeed_ * speedRate;

	player->MoveAvoid(avoidDirection_, currentSpeed);

	/*if (timer_ / duration_ > 0.8f)
	{
		player->SetTargetRoll({0.0f, 0.0f, 0.0f});
	}*/
	
	if (player->GetJustAvoidAccept())
	{
		if (timer_ >= justDuration_)
		{
			player->SetJustAvoidAccept(false);
		}
	}

	if (timer_ >= duration_)
	{
		player->SetRotate(Vector3(0.0f, 0.0f, 0.0f));
		
		/*player->ClearLockOn();*/
		player->ChangeState(std::make_unique<PlayerIdleState>());
		
	}
}

void PlayerAvoidState::Draw(Player* player)
{
	(void)player;
}

void PlayerAvoidState::Finalize(Player * player)
{
	/*player->SetTargetRoll({0.0f, 0.0f, 0.0f});*/
	
	player->SetIsHit(true);
}

void PlayerJustAvoidState::Initialize(Player* player)
{
	shotCommand_ = std::make_unique<ShotCommand>(
		player->GetInputHandler());

	justAvoidDarken_->SetSetting(setting);
	justAvoidDarken_->Play();

	player->SetRotate(Vector3(0.0f, 0.0f, 0.0f));
	player->SetIsHit(false);
	player->SetJustAvoidAccept(false);
}

void PlayerJustAvoidState::Update(Player* player, const float& deltaTime)
{
	IInputHandler* handler = player->GetInputHandler();

	timer_ += deltaTime;

	

	if (timer_ < duration_)
	{
		float progress = timer_ / duration_;
		if (progress > 1.0f) progress = 1.0f;

		float oneRotation = 2.0f * pi;
		float currentRoll = 0.0f;

		if (avoidDirection_.x < 0.0f)
		{
			currentRoll = progress * oneRotation; // 左回転
		} else
		{
			currentRoll = -progress * oneRotation;  // 右回転
		}

		Vector3 currentRotate = player->GetRotate();
		currentRotate.y = currentRoll;
		player->SetRotate(currentRotate);

		float speedRate = 1.0f - (progress * progress);

		float currentSpeed = avoidSpeed_ * speedRate;

		player->MoveJustAvoid(avoidDirection_, currentSpeed);
	}
	else
	{
		if (handler->IsActionTriggered(InputAction::Shot))
		{
			shotCommand_->Execute(player);
			isCounter_ = true;
			return; // ★ 追加: Shot()内部でChangeStateされる
		}
		if (handler->IsActionTriggered(InputAction::Melee))
		{
			player->CounterMeleeAttack();
			return;
		}
	}

	if (timer_ >= waitDuration_ || isCounter_)
	{
		player->SetRotate(Vector3(0.0f, 0.0f, 0.0f));
		//player->StopJustAvoid(0.05f);
		/*player->ClearLockOn();*/
		player->ChangeState(std::make_unique<PlayerIdleState>());
	}
}

void PlayerJustAvoidState::Draw(Player * player)
{

}

void PlayerJustAvoidState::Finalize(Player * player)
{
	player->SetIsHit(true);
}

void PlayerMeleeAttackState::Initialize(Player* player)
{
	comboIndex_ = 0;
	player->SetMeleeHandVisible(true);
	BeginAttack(player);
}

void PlayerMeleeAttackState::BeginAttack(Player* player)
{
	phase_ = AttackPhase::Windup;
	timer_ = 0.0f;
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });
	// 各段でfalseからtrueへ切り替え、同じ敵へのヒット履歴をリセットする。
	player->SetAttackColliderActive(false);
	player->ResetMeleeAttackDamage();
	player->SetMeleeHandTranslate(kRestPosition_);
	UpdateAttackDirection(player);
}

void PlayerMeleeAttackState::UpdateAttackDirection(Player* player)
{

	Vector3 currentForward = TransformNormal(
		{ 0.0f, 0.0f, 1.0f }, player->GetParentWorldTransform()
			? player->GetParentWorldTransform()->worldMatrix_ : MakeIdentity4x4());
	attackDirection_ = Length(currentForward) > 0.0001f
		? Normalize(currentForward)
		: Vector3{ 0.0f, 0.0f, 1.0f };

	if (player->HasNearestEnemy())
	{
		const Vector3 toEnemy =
			player->GetNearestEnemyPosition() - player->GetWorldPosition();
		const float distanceToEnemy = Length(toEnemy);
		if (distanceToEnemy > 0.0001f)
		{
			attackDirection_ = Normalize(toEnemy);
		}
	}

	player->SetMeleeAttackDirection(attackDirection_);
}

Vector3 PlayerMeleeAttackState::GetWindupPosition() const
{
	switch (comboIndex_)
	{
	case 1:
		return { -1.4f, 0.0f, 0.3f };
	case 2:
		return { 0.8f, 1.5f, -0.2f };
	default:
		return { 0.8f, 0.0f, -0.2f };
	}
}

Vector3 PlayerMeleeAttackState::GetAttackEndPosition() const
{
	switch (comboIndex_)
	{
	case 1:
		return { 1.6f, 0.0f, 3.8f };
	case 2:
		return { 0.8f, -0.5f, 4.5f };
	default:
		return { 0.8f, 0.0f, 4.0f };
	}
}

void PlayerMeleeAttackState::Update(Player* player, const float& deltaTime)
{
	IInputHandler* handler = player->GetInputHandler();
	const Vector3 windupPosition = GetWindupPosition();
	const Vector3 attackEndPosition = GetAttackEndPosition();
	timer_ += deltaTime;

	switch (phase_)
	{
	case AttackPhase::Windup:
		player->SetVelocity({ 0.0f, 0.0f, 0.0f });
		player->SetMeleeHandTranslate(Lerp(
			kRestPosition_, windupPosition,
			std::min(timer_ / kWindupDuration_, 1.0f)));
		if (timer_ >= kWindupDuration_)
		{
			phase_ = AttackPhase::Attack;
			timer_ = 0.0f;
			player->SetAttackColliderActive(true);
		}
		break;
	case AttackPhase::Attack:
		player->SetMeleeHandTranslate(Lerp(
			windupPosition, attackEndPosition,
			std::min(timer_ / kAttackDuration_, 1.0f)));
		if (timer_ >= kAttackDuration_)
		{
			phase_ = AttackPhase::FollowUpWindow;
			timer_ = 0.0f;
			player->SetAttackColliderActive(false);
		}
		break;
	case AttackPhase::FollowUpWindow:
		player->SetMeleeHandTranslate(Lerp(
			attackEndPosition, kRestPosition_,
			std::min(timer_ / kFollowUpDuration_, 1.0f)));

		// 通常コンボより短い受付時間内で、押した瞬間だけ追撃を発動する。
		if (timer_ >= kFollowUpInputStart_ &&
			timer_ <= kFollowUpDuration_ &&
			handler->IsActionTriggered(InputAction::Shot))
		{
			UpdateAttackDirection(player);
			player->MeleeFollowUpAttack();
			phase_ = AttackPhase::FollowUpAttack;
			timer_ = 0.0f;
			break;
		}

		if (timer_ >= kFollowUpDuration_)
		{
			phase_ = AttackPhase::ComboWindow;
			timer_ = 0.0f;
		}
		break;
	case AttackPhase::FollowUpAttack:
	{
		// 短い突き動作に近接判定を持たせ、弾は生成しない。
		const float progress =
			std::min(timer_ / kFollowUpAttackDuration_, 1.0f);
		const float thrust = 1.0f - std::abs(progress * 2.0f - 1.0f);
		player->SetMeleeHandTranslate(Lerp(
			kRestPosition_, attackEndPosition, thrust));

		if (timer_ >= kFollowUpAttackDuration_)
		{
			player->SetAttackColliderActive(false);
			phase_ = AttackPhase::ComboWindow;
			timer_ = 0.0f;
		}
		break;
	}
	case AttackPhase::ComboWindow:
		player->SetMeleeHandTranslate(kRestPosition_);

		if (comboIndex_ + 1 < kMaxComboCount_ &&
			handler->IsActionTriggered(InputAction::Melee))
		{
			++comboIndex_;
			BeginAttack(player);
			break;
		}

		if (timer_ >= kComboInputDuration_)
		{
			player->ChangeState(std::make_unique<PlayerIdleState>());
			return;
		}
		break;
	}
}

void PlayerMeleeAttackState::Draw(Player* player) { (void)player; }

void PlayerMeleeAttackState::Finalize(Player* player)
{
	player->SetAttackColliderActive(false);
	player->SetMeleeHandVisible(false);
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void PlayerCounterMeleeState::Initialize(Player* player)
{
	timer_ = 0.0f;
	startPosition_ = player->GetTranslate();
	isApproachFinished_ = false;
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });
	player->SetAttackColliderActive(false);
	player->SetMeleeHandVisible(false);
}

void PlayerCounterMeleeState::Update(Player* player, const float& deltaTime)
{
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });

	// 接近完了フレームの画面内制限とWorldTransform更新を待ってから攻撃へ移る。
	if (isApproachFinished_)
	{
		player->MeleeAttack();
		return;
	}

	timer_ += deltaTime;
	const float progress = std::min(timer_ / kApproachDuration_, 1.0f);
	const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
	const Vector3 targetPosition = GetTargetLocalPosition(player);
	player->SetTranslate(Lerp(startPosition_, targetPosition, smoothProgress));

	if (progress >= 1.0f)
	{
		isApproachFinished_ = true;
	}
}

void PlayerCounterMeleeState::Draw(Player* player)
{
	(void)player;
}

void PlayerCounterMeleeState::Finalize(Player* player)
{
	player->SetVelocity({ 0.0f, 0.0f, 0.0f });
}

Vector3 PlayerCounterMeleeState::GetTargetLocalPosition(Player* player) const
{
	Vector3 targetPosition = player->GetNearestEnemyPosition();
	if (const WorldTransform* parent = player->GetParentWorldTransform())
	{
		targetPosition = TransformPosition(
			targetPosition, Inverse(parent->worldMatrix_));
	}

	// カウンター接近は通常移動と同じ画面平面上だけで行う。
	// 奥行きは維持し、敵との近接間合いは既存のZ配置に任せる。
	targetPosition.z = startPosition_.z;
	return targetPosition;
}
