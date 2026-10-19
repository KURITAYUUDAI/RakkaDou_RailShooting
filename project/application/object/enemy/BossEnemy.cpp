#include "BossEnemy.h"

#include "BulletManager.h"
#include "DebugDrawManager.h"

void BossEnemy::InitializeSpecific(const EnemyCreateConfig& config)
{
	(void)config;
	ModelManager::GetInstance()->LoadModel("", "sphere.obj");
	ModelManager::GetInstance()->LoadModel("RightHand", "RightHand.obj");
	ModelManager::GetInstance()->LoadModel("LeftHand", "LeftHand.obj");
	GetObject3d()->SetModel("sphere.obj");

	objectRightHand_ = std::make_unique<Object3d>();
	objectRightHand_->Initialize();
	objectRightHand_->SetModel("RightHand.obj");
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
	state_->Update(this, deltaTime);
	SetTranslate(GetTranslate() + velocity_ * deltaTime);
}

void BossEnemy::UpdateAfterTransform()
{
	ApplyVisualColor();
	const float chargeProgress = GetAttackChargeProgress();
	objectRightHand_->SetColor({
		1.0f, 1.0f - chargeProgress, 1.0f - chargeProgress, 1.0f });
	objectRightHand_->SetTransform(rightHandTransform_);
	objectRightHand_->Update();
	colliderAttack_->UpdateWorldPosition();
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
	if (colliderAttack_->GetShape() == ColliderShape::AABB)
	{
		DebugDrawManager::GetInstance()->AddBox(colliderAttack_->GetWorldPosition(),
			colliderAttack_->GetSize(), { 1.0f, 0.0f, 1.0f, 1.0f });
	}
	else
	{
		DebugDrawManager::GetInstance()->AddSphere(colliderAttack_->GetWorldPosition(),
			colliderAttack_->GetRadius(), { 1.0f, 0.0f, 1.0f, 1.0f }, 8);
	}
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
