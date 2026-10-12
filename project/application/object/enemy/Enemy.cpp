#include "Enemy.h"
#include "Logger.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include "CameraManager.h"
#include "BulletManager.h"
#include "DebugDrawManager.h"
#include "EnemyEvent.h"
#include "PlayerEvent.h"
#include "Player.h"

#include "Dict_Engine/tool/effect/DissolveManager.h"

void Enemy::Initialize()
{
	deltaTime_ = 1.0f / 60.0f; // 仮の値。実際のゲームループで更新されるべ

	PSOManager::GetInstance()->RegisterEnvironmentPSO();

	ModelManager::GetInstance()->LoadModel("", "sphere.obj");
	ModelManager::GetInstance()->LoadModel("RightHand", "RightHand.obj");
	ModelManager::GetInstance()->LoadModel("LeftHand", "LeftHand.obj");

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel("sphere.obj");
	object3d_->SetEnvironmentCoefficient(0.2f);

	collider_ = std::make_unique<Collider>();
	collider_->SetOwner(this);
	collider_->SetRadius(2.0f);
	collider_->SetAttribute(CollisionAttribute::Enemy);
	collider_->SetMask(CollisionAttribute::Enemy);
	collider_->SetDamage(1);

	objectRightHand_ = std::make_unique<Object3d>();
	objectRightHand_->Initialize();
	objectRightHand_->SetModel("RightHand.obj");
	objectRightHand_->SetEnvironmentCoefficient(0.2f);
	objectRightHand_->SetShininess(0.1f);
	objectRightHand_->SetAlphaReference(0.1f);
	objectRightHand_->SetParent(object3d_->GetWorldTransform());

	colliderAttack_ = std::make_unique<Collider>();
	colliderAttack_->SetOwner(this);
	colliderAttack_->SetOnCollision(
		[this](Collider* self, Collider* other, const CollisionContact& contact)
		{
			this->OnCollision(self, other, contact);
		}
	);
	colliderAttack_->SetRadius(2.0f);
	colliderAttack_->SetAttribute(CollisionAttribute::EnemyAttack);
	colliderAttack_->SetMask(CollisionAttribute::Enemy);
	colliderAttack_->SetDamage(1);
	colliderAttack_->SetParent(objectRightHand_->GetWorldTransform());
	colliderAttack_->SetLocalPosition({ 0.0f, 0.0f, -3.0f });

	transform_.scale = { 2.0f, 2.0f, 2.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 30.0f };

	rightHandTransform_.scale = { 1.0f, 1.0f, 1.0f };
	rightHandTransform_.rotate = { -40.0f / 180.0f * pi, 0.0f, 0.0f };
	rightHandTransform_.translate = { -2.5f, 0.0, 0.0f };

	hitPoint_ = kMaxHitPoint;

	slashEmitter_ = std::make_unique<ParticleEmitter>();
	slashEmitter_->Initialize("slash",
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 3, 0.2f);

	dissolveParams_.threshold = 0.0f;
	dissolveParams_.edgeColor = { 2.0f, 0.3f, 0.3f, 1.0f };

	ChangeState(std::make_unique<EnemyBossBattleState>());
}

void Enemy::EventDispatch()
{
	eventSubscriber_.Initialize(eventBus_);

	eventSubscriber_.Subscribe<PlayerWorldPositionEvent>(
		[this](const PlayerWorldPositionEvent& event)
		{
			playerWorldPosition_ = event.worldPosition;
		}
	);

	eventBus_->Publish(EnemyAppierEvent
		{
			 .enemyID = enemyID_,

			.currentHitPoint = hitPoint_,
			.previousHitPoint = hitPoint_,
			.maxHitPoint = kMaxHitPoint,

			.screenPosition = GetScreenPosition(),

			.isVisible = !isDead_,

			.displayType = hpGageDisplayType_,
			.screenBossPriority = screenBossPriority_
		}
	);

	eventBus_->Dispatch();
}

void Enemy::Update(const float& deltaTime)
{
	deltaTime_ = deltaTime;
	const Vector3 previousWorldPosition = GetWorldPosition();

	state_->Update(this, deltaTime);

	transform_.translate += velocity_ * deltaTime;

#ifdef _DEBUG
	ImGui::Begin("EnemySetting");
	bool isDraw = isDraw_;
	if (ImGui::Checkbox("DrawEnemy", &isDraw))
	{
		isDraw_ = isDraw;
	}

	ImGui::InputInt("HitPoint", &hitPoint_);
	Vector2 screenPos = GetScreenPosition();
	ImGui::InputFloat2("ScreenPos", &screenPos.x);
	ImGui::InputFloat3("PlayerPos", &playerWorldPosition_.x);

	float dissolveThreshold = dissolveParams_.threshold;
	Vector4 dissolveEdgeColor = dissolveParams_.edgeColor;
	if (ImGui::SliderFloat("threshold", &dissolveThreshold, 0.0f, 1.0f))
	{
		SetThreshold(dissolveThreshold);
	}
	if (ImGui::DragFloat4("EdgeColor", &dissolveEdgeColor.x, 0.1f, 0.0f, 10.0f))
	{
		SetEdgeColor(dissolveEdgeColor);
	}

	EulerTransform transformRightHand = rightHandTransform_;
	if(ImGui::DragFloat3("Right Hand Position", &transformRightHand.translate.x, 0.1f))
	{
		rightHandTransform_.translate = transformRightHand.translate;
	}
	if(ImGui::DragFloat3("Right Hand Rotation", &transformRightHand.rotate.x, 0.1f))
	{
		rightHandTransform_.rotate = transformRightHand.rotate;
	}

	

	ImGui::End();
#endif

	object3d_->SetTransform(transform_);
	object3d_->Update();
	const Vector3 currentWorldPosition = GetWorldPosition();
	if (hasWorldVelocitySample_ && deltaTime_ > 0.000001f)
	{
		worldVelocity_ = (currentWorldPosition - previousWorldPosition) / deltaTime_;
	}
	else
	{
		worldVelocity_ = {};
	}
	hasWorldVelocitySample_ = true;

	objectRightHand_->SetTransform(rightHandTransform_);
	objectRightHand_->Update();

	if (damageTimer_ > 0.0f)
	{
		damageTimer_ -= kDeltaTime;
	}
	if (damageTimer_ < 0.0f)
	{
		damageTimer_ = 0.0f;
		// isPlayHitSE_ = false;
	}

	collider_->SetWorldPosition(GetWorldPosition());

	colliderAttack_->UpdateWorldPosition();

	eventBus_->Publish(EnemyScreenPositionEvent
		{
			.enemyID = enemyID_,
			.screenPosition = GetScreenPosition(),
			.isVisible = !isDead_
		}
	);

	eventBus_->Publish(EnemyBattlePhaseEvent
		{
			.enemyID = enemyID_,
			.phase = battlePhase_,
			.canReceiveMeleeApproach = CanReceiveMeleeApproach(),
		}
	);
}

void Enemy::Draw()
{
	object3d_->SetPsoName(psoName_);
	object3d_->SetBlendMode(blendMode_);
	object3d_->SetFillMode(fillMode_);
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(5, environmentTextureIndex_);

	DissolveManager::GetInstance()->SetCbufferDissolveResource(6, dissolveParams_);
	DissolveManager::GetInstance()->SetCbufferMaskTexture(7, 1);

	if (isDraw_)
	{
		if (static_cast<int>(damageTimer_ * 60.0f) % 5 == 0)
		{
			object3d_->Draw();
			objectRightHand_->Draw();
		}
	}

#ifdef _DEBUG
	if (collider_->GetShape() == ColliderShape::AABB)
	{
		DebugDrawManager::GetInstance()->AddBox(collider_->GetWorldPosition(),
			collider_->GetSize(), { 1.0f, 0.0f, 1.0f, 1.0f });
	}
	else
	{
		DebugDrawManager::GetInstance()->AddSphere(collider_->GetWorldPosition(),
			collider_->GetRadius(), { 1.0f, 0.0f, 1.0f, 1.0f }, 8);
	}

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

void Enemy::Finalize()
{
	eventSubscriber_.Finalize();
}

void Enemy::ChangeState(std::unique_ptr<IEnemyState> newState)
{
	state_ = std::move(newState);
	state_->Initialize(this);
}

void Enemy::OnCollision(
	Collider* self, Collider* other, const CollisionContact& contact)
{
	if (other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::Player))
	{
		isAttackColliderActive_ = false;
		return;
	}

	// Player本体の近接攻撃はPlayer::OnCollisionで一度だけ処理する。
	// 同じPlayerAttack属性を持つ弾は、従来どおり下の処理でダメージを受ける。
	if (other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::PlayerAttack) &&
		dynamic_cast<Player*>(other->GetOwner()) != nullptr)
	{
		return;
	}

	if (damageTimer_ == 0.0f && other->GetDamage() > 0)
	{
		Damage(other->GetDamage(), contact.position);
		damageTimer_ = kDamageInvincible_;
		//PlaySEHit();
	}
	if (hitPoint_ <= 0)
	{
		isDead_ = true;
		//PlaySEDead();
	}
}

void Enemy::Damage(int damage, const Vector3& hitPosition)
{
	const int previousHP = hitPoint_;

	hitPoint_ -= damage;

	if (hitPoint_ < 0)
	{
		hitPoint_ = 0;
	}

	if (hitPoint_ == previousHP)
	{
		return;
	}

	slashEmitter_->SetTranslate(hitPosition);
	slashEmitter_->EmitSlash();

	if (eventBus_)
	{
		eventBus_->Publish(EnemyHPChangeEvent
			{
				.enemyID = enemyID_,
				.currentHitPoint = hitPoint_,
				.previousHitPoint = previousHP,
				.maxHitPoint = kMaxHitPoint
			}
		);
	}
}

void Enemy::Move(const float& directionX, const float& directionY)
{
	float targetVelocityX = maxSpeed_.x * directionX;
	float targetVelocityY = maxSpeed_.y * directionY;

	velocity_.x += (targetVelocityX - velocity_.x) * lerpFactor_;
	velocity_.y += (targetVelocityY - velocity_.y) * lerpFactor_;
}

void Enemy::Decelerate()
{
	velocity_.x += (0.0f - velocity_.x) * lerpFactor_;
	velocity_.y += (0.0f - velocity_.y) * lerpFactor_;
}

void Enemy::Shot()
{
	FireProjectile();
	ChangeState(std::make_unique<EnemyShotState>());
}

void Enemy::FireProjectile()
{
	Vector3 bulletDirection = { 0.0f, 0.0f, -1.0f };
	Vector3 toPlayer = playerWorldPosition_ - GetWorldPosition();
	if (Length(toPlayer) > 0.0001f)
	{
		bulletDirection = Normalize(toPlayer);
	}

	/*bulletDirection = Normalize(TransformNormal(bulletDirection, object3d_->GetWorldTransform()->worldMatrix_));
	*/
	BulletManager::GetInstance()->CreateEnemyBullet(GetWorldPosition(), bulletDirection * bulletSpeed_);
}

void Enemy::Attack()
{
	ChangeState(std::make_unique<EnemyAttackState>());
}

bool Enemy::CanReceiveMeleeApproach() const
{
	return battlePhase_ == EnemyBattlePhase::CloseWait ||
		battlePhase_ == EnemyBattlePhase::CloseAttack ||
		battlePhase_ == EnemyBattlePhase::Recovery;
}

const Vector3 Enemy::GetWorldPosition() const
{
	
	Matrix4x4 worldMatrix = object3d_->GetWorldTransform()->worldMatrix_;
	
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldMatrix.m[3][0];
	worldPos.y = worldMatrix.m[3][1];
	worldPos.z = worldMatrix.m[3][2];

	return worldPos;
}

Vector3 Enemy::GetPlayerLocalPosition() const
{
	WorldTransform* parent = object3d_->GetWorldTransform()->parent_;
	if (!parent)
	{
		return playerWorldPosition_;
	}
	return TransformPosition(playerWorldPosition_, Inverse(parent->worldMatrix_));
}

const Vector2 Enemy::GetScreenPosition() const
{
	Matrix4x4 worldMatrix = object3d_->GetWorldTransform()->worldMatrix_;
	worldMatrix.m[3][1] += 1.0f;
	worldMatrix.m[3][0] += 1.0f;
	Vector3 worldPos = { worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };

	Matrix4x4 vpMatrix = CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix();
	Matrix4x4 viewport = DirectXBase::GetInstance()->GetViewportMatrix();
	Matrix4x4 screenMatrix = vpMatrix * viewport;

	// スクリーン座標を入れる変数
	Vector3 screenPos = TransformPosition(worldPos, screenMatrix);
	return {screenPos.x, screenPos.y};
}

const Vector3 Enemy::GetWorldRotate() const
{
	Matrix4x4 worldMatrix = object3d_->GetWorldTransform()->worldMatrix_;

	Vector3 worldRotEuler;
	// 行列からエウラー角を計算する（一般的な公式に基づく抽出）
	// ※アークサイン等を使うため、XYZの回転順序に依存します
	worldRotEuler.y = std::asin(-worldMatrix.m[0][2]);
	if (std::cos(worldRotEuler.y) > 0.0001f) {
		worldRotEuler.x = std::atan2(worldMatrix.m[1][2], worldMatrix.m[2][2]);
		worldRotEuler.z = std::atan2(worldMatrix.m[0][1], worldMatrix.m[0][0]);
	} else {
		worldRotEuler.x = std::atan2(-worldMatrix.m[2][1], worldMatrix.m[1][1]);
		worldRotEuler.z = 0.0f;
	}

	return worldRotEuler;
}

void Enemy::SetParent(WorldTransform* worldTransform)
{
	object3d_->SetParent(worldTransform);
}
