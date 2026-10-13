#define NOMINMAX
#include "Player.h"
#include "Logger.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include "InputManager.h"
#include "BulletManager.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "PlayerEvent.h"
#include "enemy/EnemyEvent.h"
#include "enemy/Enemy.h"
#include "enemy/EnemyManager.h"

#include <cmath>
#include <limits>

#include "Dict_Engine/tool/effect/DissolveManager.h"
#include "time/DeltaTimeManager.h"
#include "PostEffectManager.h"
#include "SceneManager.h"

#include "PrimitiveManager.h"
#include "ParticleManager.h"

void Player::Initialize(Mode mode)
{
	mode_ = mode;
	deltaTime_ = 1.0f / 60.0f; // 仮の値。実際のゲームループで更新されるべ

	selector_.GetKeyboardHandler()->AssignKey("shot", DIK_SPACE);
	selector_.GetKeyboardHandler()->AssignKey("avoid", DIK_E);
	selector_.GetKeyboardHandler()->AssignKey("lockOn", DIK_LSHIFT);   // 追加
	selector_.GetKeyboardHandler()->AssignKey("melee", DIK_B);

	selector_.GetGamepadHandler()->AssignKey("shot", XINPUT_GAMEPAD_RIGHT_SHOULDER);
	selector_.GetGamepadHandler()->AssignKey("avoid", XINPUT_GAMEPAD_X);
	selector_.GetGamepadHandler()->AssignKey("lockOn", XINPUT_GAMEPAD_LEFT_SHOULDER); // 追加
	selector_.GetGamepadHandler()->AssignKey("melee", XINPUT_GAMEPAD_B);

	ModelManager::GetInstance()->LoadModel("", "sphere.obj");
	ModelManager::GetInstance()->LoadModel("player", "player.obj");
	ModelManager::GetInstance()->LoadModel("Animation", "walk.gltf");
	ModelManager::GetInstance()->LoadModel("RightHand", "RightHand.obj");
	ModelManager::GetInstance()->LoadModel("LeftHand", "LeftHand.obj");
	ModelManager::GetInstance()->LoadModel("playerBullet", "playerBullet.obj");

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel("player.obj");
	object3d_->SetEnvironmentCoefficient(0.2f);

	objectMeleeHand_ = std::make_unique<Object3d>();
	objectMeleeHand_->Initialize();
	objectMeleeHand_->SetModel("RightHand.obj");
	objectMeleeHand_->SetPsoName("Environment");
	objectMeleeHand_->SetEnvironmentCoefficient(0.2f);
	objectMeleeHand_->SetParent(object3d_->GetWorldTransform());
	meleeHandTransform_.scale = { 0.35f, 0.35f, 0.35f };
	meleeHandTransform_.rotate = { 0.0f, pi, 0.0f };
	meleeHandTransform_.translate = { 0.8f, 0.0f, 0.5f };

	objectLeftHand_ = std::make_unique<Object3d>();
	objectLeftHand_->Initialize();
	objectLeftHand_->SetModel("LeftHand.obj");
	objectLeftHand_->SetPsoName("Environment");
	objectLeftHand_->SetEnvironmentCoefficient(0.2f);
	objectLeftHand_->SetParent(object3d_->GetWorldTransform());
	leftHandTransform_.scale = { 0.35f, 0.35f, 0.35f };
	leftHandTransform_.rotate = { 0.0f, 0.0f, -pi / 2.0f };
	leftHandTransform_.translate = { -1.4f, 0.0f, 0.0f };

	objectMeleeFollowUpEffect_ = std::make_unique<Object3d>();
	objectMeleeFollowUpEffect_->Initialize();
	objectMeleeFollowUpEffect_->SetModel("playerBullet.obj");
	objectMeleeFollowUpEffect_->SetEnableLighting(false);
	objectMeleeFollowUpEffect_->SetBlendMode(PSOManager::BlendMode::Add);
	objectMeleeFollowUpEffect_->SetColor({ 2.0f, 2.0f, 2.0f, 0.0f });
	meleeFollowUpEffectTransform_.scale = { 0.75f, 0.75f, 0.75f };
	meleeFollowUpEffectTransform_.rotate = { 0.0f, 0.0f, 0.0f };
	meleeFollowUpEffectTransform_.translate = {};

	/*animation_ = LoadAnimationFile("Animation", "walk.gltf");
	animationTime = 0.0f;

	skeleton_ = CreateSkeleton(object3d_->GetModel()->GetRootNode(0));

	skinCluster_ = CreateSkinCluster(skeleton_, object3d_->GetModel()->GetMesh(0), skinClusterHeap_);*/

	collider_ = std::make_unique<Collider>();
	collider_->SetOwner(this);
	collider_->SetShape(ColliderShape::AABB);
	collider_->SetSize({ 2.0f, 2.0f, 2.0f });
	collider_->SetAttribute(CollisionAttribute::Player);
	collider_->SetMask(CollisionAttribute::Player);

	colliderAttack_ = std::make_unique<Collider>();
	colliderAttack_->SetOwner(this);
	colliderAttack_->SetOnCollision(
		[this](Collider* self, Collider* other, const CollisionContact& contact)
		{ OnCollision(self, other, contact); });
	colliderAttack_->SetRadius(2.0f);
	colliderAttack_->SetAttribute(CollisionAttribute::PlayerAttack);
	colliderAttack_->SetMask(CollisionAttribute::Player);
	colliderAttack_->SetDamage(kMeleeDamage_);

	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, -3.0f, -7.5f };
	if (mode_ == Mode::TitleFalling)
	{
		transform_.rotate = { pi / 2.0f, 0.0f, 0.0f };
		transform_.translate = { 0.0f, 0.0f, -1.85f };

		meleeHandTransform_.scale = { 0.35f, 0.35f, 0.35f };
		meleeHandTransform_.rotate = { 0.0f, -1.84f, 0.05f };
		meleeHandTransform_.translate = { 1.2f, 0.0f, 0.0f };

		leftHandTransform_.scale = { 0.35f, 0.35f, 0.35f };
		leftHandTransform_.rotate = { 0.0f, 1.84f, -0.05f };
		leftHandTransform_.translate = { -1.2f, 0.0f, 0.0f };
	}

	hitPoint_ = kMaxHitPoint;

	dissolveParams_.threshold = 0.0f;
	dissolveParams_.edgeColor = { 0.5f, 0.5f, 2.0f, 1.0f };

	justAvoidDarken_ = std::make_unique<JustAvoidDarken>(
		SceneManager::GetInstance()->GetPostEffectController());

	PrimitiveManager::RingConfig ringConfig;
	ringConfig.segments = 32;
	ringConfig.innerRadius = 1.5f;
	ringConfig.outerRadius = 2.0f;
	ringConfig.innerColor = { 0.0f, 1.0f, 1.0f, 0.2f };
	ringConfig.outerColor = { 0.0f, 5.0f, 5.0f, 1.0f };
	ringConfig.uvScaleU = 2.0f;
	ringConfig.uvScaleV = 0.1f;
	ringConfig.startAngle = 0.0f;
	ringConfig.endAngle = 2.0f * pi;
	ringConfig.alphaFade.startFadeRange = 0.1f;
	ringConfig.alphaFade.endFadeRange = 0.1f;
	PrimitiveManager::GetInstance()->CreateRing("ring_avoid", ringConfig);

	PrimitiveManager::RingConfig chargeRingConfig;
	chargeRingConfig.segments = 48;
	chargeRingConfig.doubleSided = true;
	chargeRingConfig.innerRadius = 1.7f;
	chargeRingConfig.outerRadius = 2.2f;
	chargeRingConfig.innerColor = { 0.0f, 0.2f, 1.0f, 1.0f };
	chargeRingConfig.outerColor = { 0.0f, 0.8f, 5.0f, 1.0f };
	PrimitiveManager::GetInstance()->CreateRing("ring_charge", chargeRingConfig);

	ParticleManager::GetInstance()->CreateParticleGroup("ring_avoid", "gradationLine.png");
	ParticleManager::GetInstance()->SetModel("ring_avoid", "ring_avoid");
	ParticleManager::GetInstance()->SetIsMoveAccelerationField("ring_avoid", false);
	ParticleManager::GetInstance()->SetIsBillboard("ring_avoid", true);

	justAvoidEmitter_ = std::make_unique<ParticleEmitter>();
	justAvoidEmitter_->Initialize("ring_avoid",
		{ {1.0f, 2.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 1, 0.1f);

	objectChargeRing_ = std::make_unique<Object3d>();
	objectChargeRing_->Initialize();
	objectChargeRing_->SetModel("ring_charge");
	objectChargeRing_->SetEnableLighting(false);
	objectChargeRing_->SetColor({ 0.0f, 0.4f, 3.0f, 1.0f });
	objectChargeRing_->SetBlendMode(PSOManager::BlendMode::Add);
	chargeRingTransform_.scale = { 1.0f, 1.0f, 1.0f };
	chargeRingTransform_.rotate = { 0.0f, 0.0f, 0.0f };
	chargeRingTransform_.translate = { 0.0f, 0.0f, 0.0f };

	ChangeState(std::make_unique<PlayerIdleState>());
}

void Player::EventDispatch()
{
	eventSubscriber_.Initialize(eventBus_);

	eventSubscriber_.Subscribe<NearestEnemyInfoEvent>(
		[this](const NearestEnemyInfoEvent& event){
			hasNearestEnemy_ = event.isValid;
			cachedNearestEnemyID_ = event.enemyID;
			cachedNearestEnemyPosition_ = event.worldPosition;
		}
	);
	eventSubscriber_.Subscribe<EnemyBattlePhaseEvent>(
		[this](const EnemyBattlePhaseEvent& event)
		{
			meleeApproachAvailability_[event.enemyID] =
				event.canReceiveMeleeApproach;
		}
	);

	eventBus_->Publish(PlayerWorldPositionEvent
		{
			.worldPosition = GetWorldPosition(),
		}
	);

	eventBus_->Publish(PlayerHPChangeEvent
		{
			.currentHitPoint = hitPoint_,
			.previousHitPoint = hitPoint_,
			.maxHitPoint = kMaxHitPoint
		}
	);

	eventBus_->Dispatch();
}

void Player::Update(const float& deltaTime)
{
	if (mode_ == Mode::TitleFalling)
	{
		UpdateTitleFalling();
		return;
	}

	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Player);

	IInputHandler* handler = selector_.GetHandler();
	if (handler->IsActionPressed("lockOn"))
	{
		// 押した瞬間だけ対象を決定する。
		// 押し続けている間は最近敵が変わっても対象を切り替えない。
		if (!isLockOnHeld_)
		{
			LockOn();
		}
		isLockOnHeld_ = true;
		UpdateLockOn();
	} else
	{
		ClearLockOn();
		isLockOnHeld_ = false;
	}

	state_->Update(this, deltaTime_);

	transform_.translate += velocity_ * deltaTime_;
	/*transform_.rotate += (targetRoll_ - transform_.rotate) * 0.1f;*/

	justAvoidEmitter_->SetTranslate(GetWorldPosition());

#ifdef _DEBUG
	ImGui::Begin("PlayerSetting");
	bool isDraw = isDraw_;
	if (ImGui::Checkbox("DrawPlayer", &isDraw))
	{
		isDraw_ = isDraw;
	}

	EulerTransform transform = transform_;
	if (ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f))
	{
		transform_ = transform;
	}
	if (ImGui::DragFloat3("Rotate", &transform.rotate.x, 0.1f))
	{
		transform_ = transform;
	}
	if (ImGui::DragFloat3("Translate", &transform.translate.x, 0.1f))
	{
		transform_ = transform;
	}
	ImGui::InputFloat3 ("Velocity", &velocity_.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
	Vector3 worldPosition = GetWorldPosition();
	ImGui::InputFloat3 ("WorldPosition", &worldPosition.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
	Vector3 worldRotate = GetWorldRotate();
	ImGui::InputFloat3 ("WorldRotate", &worldRotate.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

	/*

	float environmentCoefficient = object3d_->GetEnvironmentCoefficient();
	if (ImGui::SliderFloat("Environment Coefficient", &environmentCoefficient, 0.0f, 1.0f))
	{
		object3d_->SetEnvironmentCoefficient(environmentCoefficient);
	}*/

	ImGui::Text("PlayerState: %s", typeid(*state_).name());

	ImGui::Text("LockOnEnemyID: %d", static_cast<int>(lockOnEnemyID_));

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

	ImGui::Text("isHit: %d", GetIsHit());
	ImGui::Text("justAvoidAccept : %d", justAvoidAccept_);

	ImGui::End();
#endif

	const WorldTransform* playerParent = object3d_->GetWorldTransform()->parent_;
	CameraManager::GetInstance()->LimitPlayerInFrustum(
		transform_.translate, playerParent ? &playerParent->worldMatrix_ : nullptr);

	animationTime += deltaTime;
	animationTime = std::fmod(animationTime, animation_.duration);

	ApplyAnimation(skeleton_, animation_, animationTime);
	UpdateSkeleton(skeleton_);
	UpdateSkinCluster(skinCluster_, skeleton_);

	object3d_->SetTransform(transform_);
	object3d_->Update(nullptr, nullptr, false);
	objectMeleeHand_->SetTransform(meleeHandTransform_);
	objectMeleeHand_->Update();
	UpdateMeleeFollowUpEffect(deltaTime_);
	Matrix4x4 chargeRingBillboard = CameraManager::GetInstance()->GetMainCamera()->
		GetBillboardWorldMatrix(chargeRingTransform_.scale, chargeRingTransform_.rotate,
			GetWorldPosition());
	objectChargeRing_->Update(&chargeRingBillboard, nullptr, false);

	eventBus_->Publish(PlayerWorldPositionEvent
		{
			.worldPosition = GetWorldPosition(),
		}
	);

	Vector3 lockOnTargetPosition{};
	bool isLockOnActive = false;
	if (lockOnEnemyID_ != 0)
	{
		Enemy* lockOnTarget = EnemyManager::GetInstance()->FindEnemy(lockOnEnemyID_);
		if (lockOnTarget && !lockOnTarget->GetIsDead())
		{
			lockOnTargetPosition = lockOnTarget->GetWorldPosition();
			isLockOnActive = true;
		}
	}

	eventBus_->Publish(PlayerLockOnEvent
		{
			.isLockOn = isLockOnActive,
			.targetWorldPosition = lockOnTargetPosition,
		}
	);


	if (damageTimer_ > 0.0f)
	{
		damageTimer_ -= deltaTime_;
	}
	if (damageTimer_ < 0.0f)
	{
		damageTimer_ = 0.0f;
		//isPlayHitSE_ = false;
	}

	collider_->SetWorldPosition(GetWorldPosition());
	// 大型Enemyの近接停止位置まで届くよう、攻撃判定を前方へ伸ばす。
	colliderAttack_->SetWorldPosition(GetWorldPosition() + meleeAttackDirection_ * 4.0f);
}

void Player::UpdateTitleFalling()
{
#ifdef _DEBUG
	ImGui::Begin("Title Player Setting");
	ImGui::DragFloat3("Player Scale", &transform_.scale.x, 0.05f);
	ImGui::DragFloat3("Player Rotate", &transform_.rotate.x, 0.01f);
	ImGui::DragFloat3("Player Translate", &transform_.translate.x, 0.05f);
	ImGui::Separator();
	ImGui::DragFloat3("Right Hand Scale", &meleeHandTransform_.scale.x, 0.05f);
	ImGui::DragFloat3("Right Hand Rotate", &meleeHandTransform_.rotate.x, 0.01f);
	ImGui::DragFloat3("Right Hand Translate", &meleeHandTransform_.translate.x, 0.05f);
	ImGui::Separator();
	ImGui::DragFloat3("Left Hand Scale", &leftHandTransform_.scale.x, 0.05f);
	ImGui::DragFloat3("Left Hand Rotate", &leftHandTransform_.rotate.x, 0.01f);
	ImGui::DragFloat3("Left Hand Translate", &leftHandTransform_.translate.x, 0.05f);
	ImGui::End();
#endif

	object3d_->SetTransform(transform_);
	object3d_->Update(nullptr, nullptr, false);
	objectMeleeHand_->SetTransform(meleeHandTransform_);
	objectMeleeHand_->Update();
	objectLeftHand_->SetTransform(leftHandTransform_);
	objectLeftHand_->Update();
}

void Player::Draw()
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
	DissolveManager::GetInstance()->SetCbufferMaskTexture(7, 0);

	if (isDraw_)
	{
		if (static_cast<int>(damageTimer_ * 60.0f) % 5 == 0)
		{
			object3d_->Draw(/*&skinCluster_.influenceBufferView, &skinCluster_.paletteSrvHandle.second*/);
		}
	}
	if (isDraw_ && (isMeleeHandVisible_ || mode_ == Mode::TitleFalling))
	{
		auto environmentPso = PSOManager::GetInstance()->GetPSOData(
			"Environment", blendMode_, fillMode_);
		DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(
			environmentPso.pipelineState.Get());
		DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(
			environmentPso.rootSignature.Get());
		SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(5, environmentTextureIndex_);
		DissolveManager::GetInstance()->SetCbufferDissolveResource(6, dissolveParams_);
		DissolveManager::GetInstance()->SetCbufferMaskTexture(7, 0);
		objectMeleeHand_->Draw();
	}
	if (isDraw_ && mode_ == Mode::TitleFalling)
	{
		objectLeftHand_->Draw();
	}
	if (isDraw_ && isChargeEffectActive_)
	{
		objectChargeRing_->Draw();
	}
	if (isDraw_ && isMeleeFollowUpEffectActive_)
	{
		objectMeleeFollowUpEffect_->Draw();
	}

#ifdef _DEBUG
	if (collider_->GetShape() == ColliderShape::AABB)
	{
		DebugDrawManager::GetInstance()->AddBox(collider_->GetWorldPosition(),
			collider_->GetSize(), { 1.0f, 1.0f, 1.0f, 1.0f });
	}
	else
	{
		DebugDrawManager::GetInstance()->AddSphere(collider_->GetWorldPosition(),
			collider_->GetRadius(), { 1.0f, 1.0f, 1.0f, 1.0f }, 8);
	}
	if (isAttackColliderActive_)
	{
		if (colliderAttack_->GetShape() == ColliderShape::AABB)
		{
			DebugDrawManager::GetInstance()->AddBox(colliderAttack_->GetWorldPosition(),
				colliderAttack_->GetSize(), { 0.0f, 1.0f, 1.0f, 1.0f });
		}
		else
		{
			DebugDrawManager::GetInstance()->AddSphere(colliderAttack_->GetWorldPosition(),
				colliderAttack_->GetRadius(), { 0.0f, 1.0f, 1.0f, 1.0f }, 8);
		}
	}
#endif
}

void Player::Finalize()
{
	eventSubscriber_.Finalize();
}

void Player::ChangeState(std::unique_ptr<IPlayerState> newState)
{
	if (state_)
	{
		state_->Finalize(this);
	}
	state_ = std::move(newState);
	state_->Initialize(this);
}

void Player::OnCollision(
	Collider* self, Collider* other, const CollisionContact& contact)
{
	if (self->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::PlayerAttack))
	{
		if (other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::Enemy) &&
			meleeHitEnemies_.insert(other->GetOwner()).second)
		{
			static_cast<Enemy*>(other->GetOwner())->Damage(
				self->GetDamage(), contact.position);
		}
		return;
	}

	if (justAvoidAccept_ && other->GetOwner()->GetIsHit())
	{
		JustAvoid(avoidDirection_);
		return;
	}

	if (GetIsHit() && other->GetOwner()->GetIsHit())
	{
		if (damageTimer_ == 0.0f)
		{
			Damage(other->GetDamage());
			//PlaySEHit();
		}
		if (hitPoint_ <= 0)
		{
			isDead_ = true;
			//PlaySEDead();
		}
	}
	else if(!GetIsHit() && other->GetOwner()->GetIsHit())
	{
		other->GetOwner()->SetIsHit(false);
	}

	/*if (other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::Enemy))
	{
		
	}*/
}

void Player::Damage(int damage)
{
	const int previousHP = hitPoint_;

	//if (state_->GetType() != PlayerStateType::Avoid)
	//{
	//	
	//}

	hitPoint_ -= damage;
	damageTimer_ = kDamageInvincible_;
	if (hitPoint_ < 0)
	{
		hitPoint_ = 0;
	}

	if (hitPoint_ == previousHP)
	{
		return;
	}
	if (eventBus_)
	{
		eventBus_->Publish(PlayerHPChangeEvent
			{
				.currentHitPoint = hitPoint_,
				.previousHitPoint = previousHP,
				.maxHitPoint = kMaxHitPoint
			}
		);
	}
}

void Player::UpdateLockOn()
{
	if (lockOnEnemyID_ == 0)
	{
		return;
	}

	Enemy* target = EnemyManager::GetInstance()->FindEnemy(lockOnEnemyID_);
	if (!target || target->GetIsDead())
	{
		// ボタンが押されたままなら対象を失うだけにする。
		// 別の敵の選択は、ボタンを離して再度押したときに行う。
		ClearLockOn();
	}
}

void Player::MoveHorizontal(const float& directionX, const float& directionY)
{
	float targetVelocityX = maxSpeed_.x * directionX;
	float targetVelocityY = maxSpeed_.y * directionY;

	// 現在速度を目標速度へ線形補間
	// → スティックの倒し具合に応じた速度に滑らかに追従する
	velocity_.x += (targetVelocityX - velocity_.x) * lerpFactor_;
	velocity_.y += (targetVelocityY - velocity_.y) * lerpFactor_;
}

void Player::Decelerate()
{
	velocity_.x += (0.0f - velocity_.x) * lerpFactor_;
	velocity_.y += (0.0f - velocity_.y) * lerpFactor_;
}

void Player::LockOn()
{
	if (!hasNearestEnemy_)
	{
		lockOnEnemyID_ = 0;
		return;
	}
	lockOnEnemyID_ = cachedNearestEnemyID_;
}

void Player::Shot()
{
	const Vector3 bulletDirection = CalculateShotDirection();

	if (BulletManager::GetInstance() == nullptr)
	{
		assert(false);
	}

	if (state_->GetType() == PlayerStateType::JustAvoid)
	{
		justAvoidEmitter_->Emit();
		BulletManager::GetInstance()->CreateCounterBullet(GetWorldPosition(), bulletDirection * bulletSpeed_);
	}
	else
	{
		BulletManager::GetInstance()->CreatePlayerBullet(GetWorldPosition(), bulletDirection * bulletSpeed_);
		ChangeState(std::make_unique<PlayerShotState>());
	}
}

void Player::ChargedShot()
{
	const Vector3 bulletDirection = CalculateShotDirection();

	BulletManager::GetInstance()->CreateChargedPlayerBullet(
		GetWorldPosition(), bulletDirection * bulletSpeed_);
	ChangeState(std::make_unique<PlayerShotState>());
}

void Player::MeleeFollowUpAttack()
{
	SetAttackColliderDamage(kMeleeFollowUpDamage_);
	SetAttackColliderActive(true);
	isMeleeFollowUpEffectActive_ = true;
	meleeFollowUpEffectTimer_ = 0.0f;
}

void Player::UpdateMeleeFollowUpEffect(float deltaTime)
{
	if (!objectMeleeFollowUpEffect_)
	{
		return;
	}

	if (!isMeleeFollowUpEffectActive_)
	{
		return;
	}

	meleeFollowUpEffectTimer_ += deltaTime;
	const float progress = std::min(
		meleeFollowUpEffectTimer_ / kMeleeFollowUpEffectDuration_, 1.0f);
	const float alpha = (1.0f - progress) * (1.0f - progress);
	const float scale = 0.75f + progress * 0.45f;
	meleeFollowUpEffectTransform_.scale = { scale, scale, scale };
	objectMeleeFollowUpEffect_->SetColor({ 2.0f, 2.0f, 2.0f, alpha });

	const Vector3 worldPosition = TransformPosition(
		kMeleeFollowUpEffectLocalPosition_, object3d_->GetWorldTransform()->worldMatrix_);
	const Matrix4x4 billboard = CameraManager::GetInstance()->GetMainCamera()->
		GetBillboardWorldMatrix(
			meleeFollowUpEffectTransform_.scale,
			meleeFollowUpEffectTransform_.rotate,
			worldPosition);
	objectMeleeFollowUpEffect_->Update(&billboard, nullptr, false);

	if (progress >= 1.0f)
	{
		isMeleeFollowUpEffectActive_ = false;
	}
}

Vector3 Player::CalculateShotDirection()
{
	Vector3 forward = TransformNormal(
		{ 0.0f, 0.0f, 1.0f }, object3d_->GetWorldTransform()->worldMatrix_);
	if (Length(forward) <= 0.0001f)
	{
		forward = { 0.0f, 0.0f, 1.0f };
	}
	else
	{
		forward = Normalize(forward);
	}

	if (lockOnEnemyID_ == 0)
	{
		return forward;
	}

	Enemy* target = EnemyManager::GetInstance()->FindEnemy(lockOnEnemyID_);
	if (!target || target->GetIsDead())
	{
		ClearLockOn();
		return forward;
	}

	const Vector3 shooterPosition = GetWorldPosition();
	const Vector3 targetPosition = target->GetWorldPosition();
	const Vector3 targetOffset = targetPosition - shooterPosition;
	if (Length(targetOffset) <= 0.0001f)
	{
		return forward;
	}

	const Vector3 targetVelocity = target->GetWorldVelocity();
	const float speedSquared = bulletSpeed_ * bulletSpeed_;
	const float a = Dot(targetVelocity, targetVelocity) - speedSquared;
	const float b = 2.0f * Dot(targetOffset, targetVelocity);
	const float c = Dot(targetOffset, targetOffset);
	constexpr float kEpsilon = 0.000001f;
	constexpr float kMaxInterceptTime = 5.0f;
	float interceptTime = std::numeric_limits<float>::infinity();

	if (std::abs(a) <= kEpsilon)
	{
		if (std::abs(b) > kEpsilon)
		{
			const float candidate = -c / b;
			if (candidate > kEpsilon)
			{
				interceptTime = candidate;
			}
		}
	}
	else
	{
		const float discriminant = b * b - 4.0f * a * c;
		if (discriminant >= 0.0f && std::isfinite(discriminant))
		{
			const float sqrtDiscriminant = std::sqrt(discriminant);
			const float denominator = 2.0f * a;
			const float t0 = (-b - sqrtDiscriminant) / denominator;
			const float t1 = (-b + sqrtDiscriminant) / denominator;
			if (t0 > kEpsilon)
			{
				interceptTime = t0;
			}
			if (t1 > kEpsilon && t1 < interceptTime)
			{
				interceptTime = t1;
			}
		}
	}

	Vector3 aimOffset = targetOffset;
	if (std::isfinite(interceptTime) && interceptTime <= kMaxInterceptTime)
	{
		aimOffset = targetPosition + targetVelocity * interceptTime - shooterPosition;
	}

	return Length(aimOffset) > 0.0001f ? Normalize(aimOffset) : forward;
}

void Player::StartChargeEffect()
{
	isChargeEffectActive_ = true;
}

void Player::UpdateChargeEffect(float deltaTime)
{
	(void)deltaTime;
}

void Player::StopChargeEffect()
{
	isChargeEffectActive_ = false;
}

void Player::MeleeAttack()
{
	ChangeState(std::make_unique<PlayerMeleeAttackState>());
}

void Player::CounterMeleeAttack()
{
	justAvoidEmitter_->Emit();
	if (!CanApproachNearestEnemy())
	{
		MeleeAttack();
		return;
	}
	ChangeState(std::make_unique<PlayerCounterMeleeState>());
}

bool Player::CanApproachNearestEnemy() const
{
	if (!hasNearestEnemy_)
	{
		return false;
	}
	const auto availability =
		meleeApproachAvailability_.find(cachedNearestEnemyID_);
	return availability != meleeApproachAvailability_.end() && availability->second;
}

void Player::SetAttackColliderActive(bool active)
{
	if (active && !isAttackColliderActive_)
	{
		meleeHitEnemies_.clear();
	}
	isAttackColliderActive_ = active;
}

void Player::Avoid(const Vector2& direction)
{
	ChangeState(std::make_unique<PlayerAvoidState>(direction));
}

void Player::JustAvoid(const Vector3& avoidDirection)
{
	DeltaTimeManager::GetInstance()->RequestOtherSlowMotion(DeltaTimeGroup::Player,
		0.3f, 0.05f, 1.0f, 0.05f);
	ChangeState(std::make_unique<PlayerJustAvoidState>(avoidDirection, justAvoidDarken_.get()));
}

void Player::StopJustAvoid(const float& returnRate)
{
	justAvoidDarken_->StartReturn(returnRate);
}

void Player::MoveAvoid(const Vector3 direction, float speed)
{
	velocity_ = direction * speed;
	
	// 回避方向に回転する
	
}

void Player::SetTargetRoll(const Vector3 rollRadian)
{
	targetRoll_ = rollRadian;
}

void Player::MoveJustAvoid(const Vector3 direction, float speed)
{
	velocity_ = direction * speed;
}

const Vector3 Player::GetWorldPosition() const
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

const Vector3 Player::GetWorldRotate() const
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

void Player::SetParent(WorldTransform* worldTransform)
{
	parentTransform_ = worldTransform;
	object3d_->SetParent(worldTransform);
	// EventDispatch() は最初の Update() より前にワールド座標を配信する。
	object3d_->SetTransform(transform_);
	object3d_->Update(nullptr, nullptr, false);
}

