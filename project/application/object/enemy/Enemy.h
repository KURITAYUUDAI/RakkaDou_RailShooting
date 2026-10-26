#pragma once
#include "DirectXBase.h"
#include "object3d.h"
#include "myMath.h"
#include "Model.h"
#include <memory>

#include "collision/CollisionObserver.h"
#include "collision/Collider.h"

#include "ParticleEmitter.h"

#include "EventBus.h"

#include "GamePlayUIUtility.h"

#include "Dict_Engine/tool/effect/DissolveUtility.h"
#include "EnemyUtility.h"

struct EnemyCreateConfig
{
	EnemyType type = EnemyType::Boss;
	Vector3 scale = { 2.0f, 2.0f, 2.0f };
	Vector3 position = { 0.0f, 0.0f, 7.5f };
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	int hitPoint = 500;
	float colliderRadius = 2.0f;
	float spawnFogMargin = 20.0f;
	float spawnApproachSpeed = 100.0f;
	float spawnSettleDuration = 0.5f;
	float forwardSpeed = 2.0f;
	float escapeHorizontalSpeed = 10.0f;
	float despawnHorizontalX = 12.0f;
	float fadeNearDistance = 2.0f;
	float fadeFarDistance = 7.0f;
	float minimumAlpha = 0.15f;
	SmallEnemyRole role = SmallEnemyRole::Basic;
	float shotInterval = 2.0f;
	float burstShotInterval = 0.2f;
	float bulletSpeed = 15.0f;
	float chargeDuration = 1.5f;
	float chargeSpeed = 20.0f;
	float rushDuration = 0.75f;
	float maxRushDistance = 30.0f;
	float approachDelay = 4.0f;
	float approachDuration = 1.5f;
	float approachStepDistance = 10.0f;
	float nearDuration = 4.0f;
	float escapeSpeed = 12.0f;
	float farLineZ = 12.0f;
	float nearLineZ = -2.0f;
	float despawnLineZ = -15.0f;
	int contactDamage = 1;
};

class Enemy : public ICollisionObserver
{
public:
	virtual ~Enemy() { Finalize(); }

	void Initialize(const EnemyCreateConfig& config = {});
	void EventDispatch();
	void Update(const float& deltaTime);


	void Draw();
	void Finalize();

	void OnCollision(
		Collider* self, Collider* other, const CollisionContact& contact) override;
	void Damage(int damage, const Vector3& hitPosition);

public: // Command対応

	/*void MoveAvoid(const Vector3 direction, float speed);
	void SetTargetRoll(const Vector3 rollRadian);*/

public:	//外部入出力

// Getter

	const float& GetDeltaTime() const { return deltaTime_; }

	// transform設定
	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }
	Vector3& GetLocalPositionReference() { return transform_.translate; }
	const EulerTransform& GetTransform() const { return transform_; }
	const Vector3& GetWorldVelocity() const { return worldVelocity_; }

	const Vector3 GetWorldPosition() const;
	const Vector2 GetScreenPosition() const;
	const Vector3 GetWorldRotate() const;

	// Collider設定
	Collider* GetCollider() { return collider_.get(); }
	virtual Collider* GetAttackCollider() { return nullptr; }
	virtual bool GetIsAttackColliderActive() const { return false; }
	virtual EnemyBattlePhase GetBattlePhase() const { return EnemyBattlePhase::Ranged; }
	virtual bool CanReceiveMeleeApproach() const { return true; }
	EnemyType GetEnemyType() const { return enemyType_; }
	virtual bool GetIsBoss() const { return false; }
	virtual const char* GetRoleName() const { return "Enemy"; }
	virtual const char* GetBehaviorStateName() const { return "Common"; }

	// HP
	const int& GetHitPoint() const { return hitPoint_; }
	int GetMaxHitPoint() const { return maxHitPoint_; }
	const Vector3& GetSpawnPosition() const { return basePosition_; }
	// デスフラグ
	bool GetIsDead() const { return isDead_; }
	bool IsReadyForRemoval() const { return isReadyForRemoval_; }

	// HPゲージUI設定
	EnemyID GetEnemyID() const { return enemyID_; }
	EnemyHPGageDisplayType GetHPGageDisplayType() const { return hpGageDisplayType_; }
	int GetScreenBossPriority() const { return screenBossPriority_; }
	
	// Dissolve設定
	const float& GetThreshold() { return dissolveParams_.threshold; }
	const Vector4& GetEdgeColor() { return dissolveParams_.edgeColor; }

	const Vector3& GetPlayerWorldPosition() const { return playerWorldPosition_; }
	Vector3 GetPlayerLocalPosition() const;

// Setter

	// transform設定
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
	void SetTransform(const EulerTransform& transform) { transform_ = transform; }
	void SetParent(WorldTransform* worldTransform);

	// 描画設定
	void SetEnvironmentTextureIndex(const uint32_t& srvIndex){ environmentTextureIndex_ = srvIndex; }
	// イベント設定
	void SetEventBus(EventBus* eventBus) { eventBus_ = eventBus; }

	// その他変数設定
	void SetIsHPChanged(const int isHPChanged){ isHPChanged_ = isHPChanged; }
	void SetAttackChargeProgress(float progress);
	float GetAttackChargeProgress() const { return attackChargeProgress_; }

	// HPゲージUI設定
	void SetEnemyID(EnemyID id) { enemyID_ = id; }
	void SetHPGageDisplayType(EnemyHPGageDisplayType type) { hpGageDisplayType_ = type; }
	void SetScreenBossPriority(int priority) { screenBossPriority_ = priority; }

	// Dissolve設定
	void SetThreshold(const float threshold) { dissolveParams_.threshold = threshold; }
	void SetEdgeColor(const Vector4 edgeColor) { dissolveParams_.edgeColor = edgeColor; }

protected:
	virtual void InitializeSpecific(const EnemyCreateConfig& config);
	virtual void UpdateBehavior(const float& deltaTime);
	virtual void UpdateAfterTransform();
	virtual void UpdateDebugUI();
	virtual void DrawAdditional();
	virtual void DrawAdditionalDebug();
	virtual void OnPlayerCollision();

	Object3d* GetObject3d() { return object3d_.get(); }
	WorldTransform* GetParentWorldTransform() const
	{
		return object3d_->GetWorldTransform()->parent_;
	}
	void UpdateCameraFade();
	void ApplyVisualColor(float alphaScale = 1.0f);
	bool IsDeathEffectActive() const { return isDeathEffectActive_; }
	float GetDeathEffectProgress() const;
	void MarkForRemoval()
	{
		isDead_ = true;
		isReadyForRemoval_ = true;
	}
	void SetBodyCollisionDamage(int damage) { collider_->SetDamage(damage); }

private:
	void SpawnHitCircle(const Vector3& localHitPosition);
	void BeginDeathEffect();
	void UpdateDeathEffect(float deltaTime);

	EnemyID enemyID_ = 0;

	EnemyHPGageDisplayType hpGageDisplayType_ =
		EnemyHPGageDisplayType::None;

	int screenBossPriority_ = 0;

	// イベント
	EventBus* eventBus_ = nullptr;
	EventSubscriber eventSubscriber_;
	std::vector<EventBus::SubscriptionID> subscriptionIDs_;

	float deltaTime_ = 0.0f;

	std::string psoName_ = "Environment";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	std::unique_ptr<Object3d> object3d_;
	std::unique_ptr<Collider> collider_;

	EulerTransform transform_;

	Vector3 worldVelocity_ = { 0.0f, 0.0f, 0.0f };
	bool hasWorldVelocitySample_ = false;

	// 環境テクスチャのsrvIndex
	uint32_t environmentTextureIndex_ = 0;

	bool isDraw_ = true;
	EnemyType enemyType_ = EnemyType::Boss;
	Vector3 basePosition_{};
	Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float fadeNearDistance_ = 2.0f;
	float fadeFarDistance_ = 7.0f;
	float minimumAlpha_ = 0.15f;
	float attackChargeProgress_ = 0.0f;

	// HP
	int hitPoint_ = 0;
	// 最大HP
	static inline const int kMaxHitPoint = 500;
	int maxHitPoint_ = kMaxHitPoint;

	float damageTimer_ = 0.0f;
	const float kDamageInvincible_ = 0.1f;

	// デスフラグ
	bool isDead_ = false;
	bool isDeathEffectActive_ = false;
	bool isReadyForRemoval_ = false;
	float deathEffectElapsedTime_ = 0.0f;
	static constexpr float kDeathEffectDuration_ = 0.8f;

	static constexpr float kHitCircleDuration_ = 0.25f;

	bool isHPChanged_ = false;

	Vector3 playerWorldPosition_{};

	std::unique_ptr<ParticleEmitter> slashEmitter_ = nullptr;
	ParticleParentHandle particleParentHandle_;

	DissolveParams dissolveParams_;
};
