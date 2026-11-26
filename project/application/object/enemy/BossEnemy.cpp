#define NOMINMAX
#include "BossEnemy.h"

#include "BulletManager.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "DistanceFogManager.h"
#include "EnemyEvent.h"

#include <algorithm>

void BossEnemy::InitializeSpecific(const EnemyCreateConfig& config)
{
	isDamageReactionActive_ = false;
	damageReactionTimer_ = 0.0f;
	defeatedPhase_ = DefeatedPhase::Recoil;
	defeatedPhaseTimer_ = 0.0f;
	defeatedVisualRotation_ = {};
	defeatedAngularSpeed_ = 0.0f;
	defeatedRailAdvanceSpeed_ = 0.0f;
	defeatedFogEndDistance_ = 0.0f;
	defeatedRailStartZ_ = 0.0f;
	defeatedRailTargetTravelDistance_ = 0.0f;
	defeatedRailTravelDistance_ = 0.0f;

	ModelManager::GetInstance()->LoadModel("enemy", "BossEnemy.obj");
	ModelManager::GetInstance()->LoadModel("RightHand", "RightHand.obj");
	ModelManager::GetInstance()->LoadModel("LeftHand", "LeftHand.obj");
	GetObject3d()->SetModel("BossEnemy.obj");
	GetObject3d()->SetColor(config.color);
	GetObject3d()->SetEnvironmentCoefficient(0.2f);

	objectRightHand_ = std::make_unique<Object3d>();
	objectRightHand_->Initialize();
	objectRightHand_->SetModel("RightHand.obj");
	objectRightHand_->SetTexture("enemy/BossEnemyHand.png");
	objectRightHand_->SetEnvironmentCoefficient(0.2f);
	objectRightHand_->SetShininess(0.1f);
	objectRightHand_->SetAlphaReference(0.1f);
	objectRightHand_->SetParent(GetObject3d()->GetWorldTransform());

	colliderAttack_ = std::make_unique<Collider>();
	colliderAttack_->SetOwner(this);
	colliderAttack_->SetOnCollision(
		[this](Collider* self, Collider* other, const CollisionContact& contact)
		{
			OnCollision(self, other, contact);
		});
	colliderAttack_->SetRadius(2.0f);
	colliderAttack_->SetAttribute(CollisionAttribute::EnemyAttack);
	colliderAttack_->SetMask(CollisionAttribute::Enemy);
	colliderAttack_->SetDamage(1);
	colliderAttack_->SetParent(objectRightHand_->GetWorldTransform());
	colliderAttack_->SetLocalPosition({ 0.0f, 0.0f, -3.0f });

	rightHandTransform_.scale = { 1.0f, 1.0f, 1.0f };
	rightHandTransform_.rotate = { -40.0f / 180.0f * pi, 0.0f, 0.0f };
	rightHandTransform_.translate = { -2.5f, 0.0f, 0.0f };

	ChangeState(std::make_unique<EnemyBossBattleState>());
}

void BossEnemy::UpdateBehavior(const float& deltaTime)
{
	UpdateDamageReaction(deltaTime);
	if (GetIsDead())
	{
		return;
	}

	state_->Update(this, deltaTime);
	SetTranslate(GetTranslate() + velocity_ * deltaTime);
}

void BossEnemy::Damage(int damage, const Vector3& hitPosition)
{
	const int previousHitPoint = GetHitPoint();
	Enemy::Damage(damage, hitPosition);
	if (GetHitPoint() == previousHitPoint)
	{
		return;
	}

	if (GetIsDead())
	{
		CameraManager::GetInstance()->StartShake(0.45f, 0.8f);
		return;
	}

	StartDamageReaction();
	CameraManager::GetInstance()->StartShake(0.25f, 0.35f);
}

void BossEnemy::StartDamageReaction()
{
	damageReactionTimer_ = 0.0f;
	isDamageReactionActive_ = true;
}

void BossEnemy::UpdateDamageReaction(float deltaTime)
{
	if (!isDamageReactionActive_)
	{
		return;
	}

	damageReactionTimer_ += deltaTime;
	if (damageReactionTimer_ >= kDamageReactionDuration_)
	{
		damageReactionTimer_ = 0.0f;
		isDamageReactionActive_ = false;
	}
}

float BossEnemy::CalculateDamageRecoilAngle() const
{
	if (damageReactionTimer_ < kDamageReactionImpactDuration_)
	{
		const float progress = std::clamp(
			damageReactionTimer_ / kDamageReactionImpactDuration_, 0.0f, 1.0f);
		const float inverse = 1.0f - progress;
		return kDamageReactionAngle_ * (1.0f - inverse * inverse);
	}

	const float returnDuration = std::max(
		kDamageReactionDuration_ - kDamageReactionImpactDuration_, 0.001f);
	const float progress = std::clamp(
		(damageReactionTimer_ - kDamageReactionImpactDuration_) / returnDuration,
		0.0f, 1.0f);
	const float smoothProgress = progress * progress * (3.0f - 2.0f * progress);
	return kDamageReactionAngle_ * (1.0f - smoothProgress);
}

void BossEnemy::UpdateAfterTransform()
{
	ApplyVisualColor();
	if (IsDeathEffectActive())
	{
		const float progress = GetDeathEffectProgress();
		objectRightHand_->SetColor({
			1.0f, 1.0f - progress, 1.0f, 1.0f - progress });
	}
	else
	{
		const float chargeProgress = GetAttackChargeProgress();
		objectRightHand_->SetColor({
			1.0f, 1.0f - chargeProgress, 1.0f - chargeProgress, 1.0f });
	}
	objectRightHand_->SetTransform(rightHandTransform_);
	objectRightHand_->Update();
	colliderAttack_->UpdateWorldPosition();
}

void BossEnemy::StartDefeatedSequence()
{
	PublishEvent(BossDefeatedSequenceStartedEvent
		{
			.enemyID = GetEnemyID(),
		});

	velocity_ = {};
	isDamageReactionActive_ = false;
	damageReactionTimer_ = 0.0f;
	isAttackColliderActive_ = false;
	colliderAttack_->SetDamage(0);
	SetAttackChargeProgress(0.0f);
	SetThreshold(0.0f);

	defeatedPhase_ = DefeatedPhase::Recoil;
	defeatedPhaseTimer_ = 0.0f;
	defeatedVisualRotation_ = {};
	defeatedAngularSpeed_ = 0.0f;

	const Vector3 cameraPosition =
		CameraManager::GetInstance()->GetCameraWorldPosition();
	defeatedFogEndDistance_ =
		DistanceFogManager::GetInstance()->GetEndDistance() + kDefeatedFogMargin_;
	defeatedRailStartZ_ = GetTranslate().z;
	defeatedRailTravelDistance_ = 0.0f;
	defeatedRailTargetTravelDistance_ = 0.0f;

	Vector3 sampledWorldPosition{};
	for (float travelDistance = 0.0f;
		travelDistance <= kDefeatedRailSearchLimit_;
		travelDistance += kDefeatedRailSearchStep_)
	{
		if (!TryGetRailWorldPositionAtLogicalZ(
			defeatedRailStartZ_ + travelDistance, sampledWorldPosition))
		{
			break;
		}
		if (Length(sampledWorldPosition - cameraPosition) >= defeatedFogEndDistance_)
		{
			defeatedRailTargetTravelDistance_ = travelDistance;
			break;
		}
	}

	if (defeatedRailTargetTravelDistance_ <= 0.0f)
	{
		const float currentCameraDistance =
			Length(GetWorldPosition() - cameraPosition);
		defeatedRailTargetTravelDistance_ =
			defeatedFogEndDistance_ + currentCameraDistance;
	}
	defeatedRailAdvanceSpeed_ =
		defeatedRailTargetTravelDistance_ / kDefeatedSequenceDuration_;
}

void BossEnemy::UpdateDefeatedSequence(float deltaTime)
{
	defeatedPhaseTimer_ += deltaTime;
	if (defeatedPhase_ == DefeatedPhase::Recoil)
	{
		const float progress = std::clamp(
			defeatedPhaseTimer_ / kDefeatedRecoilImpactDuration_, 0.0f, 1.0f);
		const float inverse = 1.0f - progress;
		const float eased = 1.0f - inverse * inverse * inverse;
		defeatedVisualRotation_.x = kDamageReactionAngle_ * eased;
		if (defeatedPhaseTimer_ >= kDefeatedRecoilDuration_)
		{
			defeatedPhase_ = DefeatedPhase::Spinning;
			defeatedPhaseTimer_ = 0.0f;
		}
		return;
	}

	defeatedRailTravelDistance_ = std::min(
		defeatedRailTravelDistance_ + defeatedRailAdvanceSpeed_ * deltaTime,
		defeatedRailTargetTravelDistance_);
	Vector3 position = GetTranslate();
	position.z = defeatedRailStartZ_ + defeatedRailTravelDistance_;
	SetTranslate(position);
	const float accelerationProgress = std::clamp(
		defeatedPhaseTimer_ / kDefeatedSpinAccelerationDuration_, 0.0f, 1.0f);
	const float spinDirection = kDamageReactionAngle_ >= 0.0f ? 1.0f : -1.0f;
	defeatedAngularSpeed_ = spinDirection *
		kDefeatedMaxAngularSpeed_ * accelerationProgress;
	defeatedVisualRotation_.x += defeatedAngularSpeed_ * deltaTime;

	if (defeatedRailTravelDistance_ >= defeatedRailTargetTravelDistance_)
	{
		PublishEvent(BossDefeatedSequenceFinishedEvent
			{
				.enemyID = GetEnemyID(),
			});
		MarkForRemoval();
	}
}

float BossEnemy::GetRailFollowBlend() const
{
	// のけぞりと回転で座標基準を切り替えない。
	// レール方向への移動自体はSpinningフェーズから開始する。
	return IsDeathEffectActive() ? 1.0f : 0.0f;
}

Vector3 BossEnemy::GetVisualRotationOffset() const
{
	if (IsDeathEffectActive())
	{
		return defeatedVisualRotation_;
	}
	if (isDamageReactionActive_)
	{
		return { CalculateDamageRecoilAngle(), 0.0f, 0.0f };
	}
	return {};
}

void BossEnemy::UpdateDebugUI()
{
#ifdef _DEBUG
	EulerTransform transformRightHand = rightHandTransform_;
	if (ImGui::DragFloat3("Right Hand Position", &transformRightHand.translate.x, 0.1f))
	{
		rightHandTransform_.translate = transformRightHand.translate;
	}
	if (ImGui::DragFloat3("Right Hand Rotation", &transformRightHand.rotate.x, 0.1f))
	{
		rightHandTransform_.rotate = transformRightHand.rotate;
	}
#endif
}

void BossEnemy::DrawAdditional()
{
	objectRightHand_->Draw();
}

void BossEnemy::DrawAdditionalDebug()
{
#ifdef _DEBUG
	colliderAttack_->DrawDebug({ 1.0f, 0.0f, 1.0f, 1.0f });
#endif
}

void BossEnemy::OnPlayerCollision()
{
	isAttackColliderActive_ = false;
}

void BossEnemy::ChangeState(std::unique_ptr<IEnemyState> newState)
{
	state_ = std::move(newState);
	state_->Initialize(this);
}

void BossEnemy::Move(float directionX, float directionY)
{
	const float targetVelocityX = maxSpeed_.x * directionX;
	const float targetVelocityY = maxSpeed_.y * directionY;
	velocity_.x += (targetVelocityX - velocity_.x) * lerpFactor_;
	velocity_.y += (targetVelocityY - velocity_.y) * lerpFactor_;
}

void BossEnemy::Decelerate()
{
	velocity_.x += (0.0f - velocity_.x) * lerpFactor_;
	velocity_.y += (0.0f - velocity_.y) * lerpFactor_;
}

void BossEnemy::Shot()
{
	FireProjectile();
	ChangeState(std::make_unique<EnemyShotState>());
}

void BossEnemy::FireProjectile()
{
	Vector3 bulletDirection = { 0.0f, 0.0f, -1.0f };
	const Vector3 toPlayer = GetPlayerLocalPosition() - GetTranslate();
	if (Length(toPlayer) > 0.0001f)
	{
		bulletDirection = Normalize(toPlayer);
	}
	BulletManager::GetInstance()->CreateEnemyBullet(
		GetTranslate(), bulletDirection * bulletSpeed_, GetParentWorldTransform());
}

void BossEnemy::Attack()
{
	ChangeState(std::make_unique<EnemyAttackState>());
}

bool BossEnemy::CanReceiveMeleeApproach() const
{
	return battlePhase_ == EnemyBattlePhase::CloseWait ||
		battlePhase_ == EnemyBattlePhase::CloseAttack ||
		battlePhase_ == EnemyBattlePhase::Recovery;
}
