#include "GameSceneCommand.h"
#include "Player.h"
#include "InputHandler.h"

void MoveHorizontalCommand::Execute(Player* player)
{
	Vector2 direction = inputHandler_->GetDirection();
    if (direction.x == 0.0f && direction.y == 0.0f)
    {
        player->Decelerate(); // 入力なし → 減速
    } 
    else
    {
        player->MoveHorizontal(direction.x, direction.y); // 入力あり → 加速
    }
}

void ShotCommand::Execute(Player* player)
{
	player->Shot();
}

LongPressShotCommand::Result LongPressShotCommand::Update(Player* player, float deltaTime)
{
	if (inputHandler_->IsActionPressed(InputAction::Shot))
	{
		if (!isCharging_)
		{
			isCharging_ = true;
			holdTime_ = 0.0f;
			isChargeEffectVisible_ = false;
		}
		holdTime_ += deltaTime;
		if (holdTime_ >= kChargeDuration)
		{
			if (!isChargeEffectVisible_)
			{
				player->StartChargeEffect();
				isChargeEffectVisible_ = true;
			}
			player->UpdateChargeEffect(deltaTime);
		}
		return Result::None;
	}

	// デバイスの切り替え・切断時にもチャージを確実に終了する。
	if (isCharging_)
	{
		const bool isChargedShot = holdTime_ >= kChargeDuration;
		isCharging_ = false;
		isChargeEffectVisible_ = false;
		holdTime_ = 0.0f;
		player->StopChargeEffect();
		return isChargedShot ? Result::ChargedShot : Result::NormalShot;
	}

	return Result::None;
}

void LongPressShotCommand::Cancel(Player* player)
{
	if (isCharging_)
	{
		player->StopChargeEffect();
	}
	isCharging_ = false;
	isChargeEffectVisible_ = false;
	holdTime_ = 0.0f;
}

void AvoidCommand::Execute(Player* player)
{
    Vector2 direction = inputHandler_->GetDirection();
    player->Avoid(direction);
}

void MeleeAttackCommand::Execute(Player* player)
{
	player->MeleeAttack();
}


