#define NOMINMAX
#include "Enemy.h"
#include "Logger.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "EnemyEvent.h"
#include "PlayerEvent.h"
#include "Player.h"

#include "Dict_Engine/tool/effect/DissolveManager.h"

#include <algorithm>
#include <cmath>

void Enemy::Initialize(const EnemyCreateConfig& config)
{
	deltaTime_ = 1.0f / 60.0f; // 仮の値。実際のゲームループで更新されるべ

	PSOManager::GetInstance()->RegisterEnvironmentPSO();

	ModelManager::GetInstance()->LoadModel("enemy", "enemy.obj");

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel("enemy.obj");
	object3d_->SetEnvironmentCoefficient(0.2f);
	object3d_->SetColor(config.color);

	collider_ = std::make_unique<Collider>();
	collider_->SetOwner(this);
	collider_->SetRadius(config.colliderRadius);
	collider_->SetAttribute(CollisionAttribute::Enemy);
	collider_->SetMask(CollisionAttribute::Enemy);
	collider_->SetDamage(1);

	transform_.scale = config.scale;
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = config.position;
	basePosition_ = config.position;
	baseColor_ = config.color;
	enemyType_ = config.type;
	fadeNearDistance_ = config.fadeNearDistance;
	fadeFarDistance_ = config.fadeFarDistance;
	minimumAlpha_ = std::clamp(config.minimumAlpha, 0.0f, 1.0f);
	if (!GetIsBoss())
	{
		object3d_->SetAlphaReference(0.0f);
	}

	maxHitPoint_ = std::max(config.hitPoint, 1);
	hitPoint_ = maxHitPoint_;

	slashEmitter_ = std::make_unique<ParticleEmitter>();
	slashEmitter_->Initialize("slash",
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 3, 0.2f);

	dissolveParams_.threshold = 0.0f;
	dissolveParams_.edgeColor = { 2.0f, 0.3f, 0.3f, 1.0f };


	InitializeSpecific(config);
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
			.maxHitPoint = maxHitPoint_,

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

	UpdateBehavior(deltaTime);

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

	UpdateDebugUI();

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
	UpdateAfterTransform();

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
			.phase = GetBattlePhase(),
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
			DrawAdditional();
		}
	}

#ifdef _DEBUG
	if (DebugDrawManager::GetInstance()->IsCollisionDebugDrawEnabled())
	{
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
		DrawAdditionalDebug();
	}
#endif
}

void Enemy::Finalize()
{
	eventSubscriber_.Finalize();
}

void Enemy::OnCollision(
	Collider* self, Collider* other, const CollisionContact& contact)
{
	if (other->GetAttribute() == static_cast<uint32_t>(CollisionAttribute::Player))
	{
		OnPlayerCollision();
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
	if (hitPoint_ == 0)
	{
		isDead_ = true;
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
				.maxHitPoint = maxHitPoint_
			}
		);
	}
}

void Enemy::InitializeSpecific(const EnemyCreateConfig& config)
{
	(void)config;
}

void Enemy::UpdateBehavior(const float& deltaTime)
{
	(void)deltaTime;
}

void Enemy::UpdateAfterTransform()
{
}

void Enemy::UpdateDebugUI()
{
}

void Enemy::DrawAdditional()
{
}

void Enemy::DrawAdditionalDebug()
{
}

void Enemy::OnPlayerCollision()
{
}

void Enemy::SetAttackChargeProgress(float progress)
{
	attackChargeProgress_ = std::clamp(progress, 0.0f, 1.0f);
}

void Enemy::ApplyVisualColor(float alphaScale)
{
	Vector4 color = baseColor_;
	color.x += (1.0f - color.x) * attackChargeProgress_;
	color.y *= 1.0f - attackChargeProgress_;
	color.z *= 1.0f - attackChargeProgress_;
	color.w *= alphaScale;
	object3d_->SetColor(color);
}

void Enemy::UpdateCameraFade()
{
	const Vector3 cameraPosition =
		CameraManager::GetInstance()->GetCameraWorldPosition();
	const float distance = Length(GetWorldPosition() - cameraPosition);
	const float nearDistance = std::min(fadeNearDistance_, fadeFarDistance_);
	const float farDistance = std::max(fadeNearDistance_, fadeFarDistance_);
	const float range = std::max(farDistance - nearDistance, 0.0001f);
	const float fade = std::clamp((distance - nearDistance) / range, 0.0f, 1.0f);
	ApplyVisualColor(minimumAlpha_ + (1.0f - minimumAlpha_) * fade);
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

	// カメラ平面上では透視除算の w が0になる。UI座標のためにゲームを
	// 停止させず、画面外として扱う。
	const float screenX = worldPos.x * screenMatrix.m[0][0] +
		worldPos.y * screenMatrix.m[1][0] +
		worldPos.z * screenMatrix.m[2][0] + screenMatrix.m[3][0];
	const float screenY = worldPos.x * screenMatrix.m[0][1] +
		worldPos.y * screenMatrix.m[1][1] +
		worldPos.z * screenMatrix.m[2][1] + screenMatrix.m[3][1];
	const float w = worldPos.x * screenMatrix.m[0][3] +
		worldPos.y * screenMatrix.m[1][3] +
		worldPos.z * screenMatrix.m[2][3] + screenMatrix.m[3][3];
	if (w <= 0.000001f)
	{
		return { -10000.0f, -10000.0f };
	}
	return { screenX / w, screenY / w };
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
	// EventDispatch() の画面座標計算より先にワールド行列を確定する。
	object3d_->SetTransform(transform_);
	object3d_->Update();
}
