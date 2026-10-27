#pragma once
#include "DirectXBase.h"
#include "object3d.h"
#include "myMath.h"
#include "Model.h"
#include <memory>

#include "PlayerInputAdapter.h"
#include "PlayerState.h"
#include "collision/CollisionObserver.h"
#include "collision/Collider.h"

#include "enemy/EnemyUtility.h"
#include "EventBus.h"

#include "Dict_Engine/tool/effect/DissolveUtility.h"

#include "ParticleEmitter.h"
#include "JustAvoidDarken.h"
#include <unordered_map>
#include <unordered_set>

#include "Animation/AnimationUtility.h"
#include "Animation/AnimationMemo.h"

class Player : public ICollisionObserver
{
public:
	enum class Mode
	{
		Gameplay,
		TitleFalling,
		StageEntry,
	};

	static constexpr Vector3 GetGameplayInitialPosition()
	{
		return { 0.0f, -1.9f, -7.5f };
	}

	void Initialize(Mode mode = Mode::Gameplay);
	void EventDispatch();
	void Update(const float& deltaTime);


	void Draw();
	void Finalize();

	void ChangeState(std::unique_ptr<IPlayerState> newState);

	void OnCollision(
		Collider* self, Collider* other, const CollisionContact& contact) override;

	void Damage(int damage);

	void UpdateLockOn();              // 選択済みのロックオン対象が有効か確認する
	void ClearLockOn() { lockOnEnemyID_ = 0; }
	EnemyID GetLockOnEnemyID() const { return lockOnEnemyID_; } // 追加

public:	// Command

	void MoveHorizontal(const float& directionX, const float& directionY);
	void Decelerate();
	void LockOn();
	void Shot();
	void ChargedShot();
	void MeleeFollowUpAttack();
	void StartChargeEffect();
	void UpdateChargeEffect(float deltaTime);
	void StopChargeEffect();
	void MeleeAttack();
	void CounterMeleeAttack();
	void Avoid(const Vector2& direction);
	void JustAvoid(const Vector3& avoidDirection);
	void StopJustAvoid(const float& returnRate);

public: // Command対応

	void MoveAvoid(const Vector3 direction, float speed);
	void SetTargetRoll(const Vector3 rollRadian);
	void MoveJustAvoid(const Vector3 direction, float speed);

public:	//外部入出力

	IInputHandler* GetInputHandler() { return inputAdapter_.GetHandler(); }
	
	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }
	Vector3& GetLocalPositionReference() { return transform_.translate; }
	const EulerTransform& GetTransform() const { return transform_; }
	const Vector3& GetVelocity() const { return velocity_; }

	const Vector3 GetWorldPosition() const;
	const Vector3 GetWorldRotate() const;
	const Matrix4x4& GetWorldMatrix() const { return object3d_->GetWorldTransform()->worldMatrix_; }

	Collider* GetCollider() { return collider_.get(); }
	Collider* GetAttackCollider() { return colliderAttack_.get(); }
	bool GetIsAttackColliderActive() const { return isAttackColliderActive_; }
	bool HasNearestEnemy() const { return hasNearestEnemy_; }
	const Vector3& GetNearestEnemyPosition() const { return cachedNearestEnemyPosition_; }
	bool CanApproachNearestEnemy() const;
	WorldTransform* GetParentWorldTransform() const { return parentTransform_; }

	// HP
	const int& GetHitPoint() const { return hitPoint_; }
	const int& GetMaxHitPoint() const { return kMaxHitPoint; }
	// デスフラグ
	bool GetIsDead() const { return isDead_; }

	const float& GetThreshold() { return dissolveParams_.threshold; }
	const Vector4& GetEdgeColor() { return dissolveParams_.edgeColor; }

	const bool GetJustAvoidAccept() { return justAvoidAccept_; }

	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
	void SetTransform(const EulerTransform& transform) { transform_ = transform; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

	void SetEnvironmentTextureIndex(const uint32_t& srvIndex){ environmentTextureIndex_ = srvIndex; }
	void SetEventBus(EventBus* eventBus) { eventBus_ = eventBus; }

	void SetParent(WorldTransform* worldTransform);

	void SetIsHPChanged(const int isHPChanged){ isHPChanged_ = isHPChanged; }


	void SetThreshold(const float threshold) { dissolveParams_.threshold = threshold; }
	void SetEdgeColor(const Vector4 edgeColor) { dissolveParams_.edgeColor = edgeColor; }

	void SetAvoidDirection(const Vector3& avoidDirection){ avoidDirection_ = avoidDirection; }
	void SetJustAvoidAccept(const bool justAvoidAccept) { justAvoidAccept_ = justAvoidAccept; }
	void SetAttackColliderActive(bool active);
	void SetAttackColliderDamage(int damage) { colliderAttack_->SetDamage(damage); }
	void ResetMeleeAttackDamage() { SetAttackColliderDamage(kMeleeDamage_); }
	void SetMeleeAttackDirection(const Vector3& direction) { meleeAttackDirection_ = direction; }
	void SetMeleeHandVisible(bool visible) { isMeleeHandVisible_ = visible; }
	void SetMeleeHandTranslate(const Vector3& translate) { meleeHandTransform_.translate = translate; }
	void SetTitleOpacity(float opacity);
	void FinishStageEntry();

private:
	Vector3 CalculateShotDirection();
	void UpdateParentWorldVelocity(float deltaTime);
	void UpdateMeleeFollowUpEffect(float deltaTime);
	void UpdateTitleFalling();

	// 入力ハンドル
	PlayerInputAdapter inputAdapter_;

	// 現在の状態
	std::unique_ptr<IPlayerState> state_;

	// イベント
	EventBus* eventBus_ = nullptr;
	EventSubscriber eventSubscriber_;

	
	float deltaTime_ = 0.0f;

	std::string psoName_ = "Environment";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	std::unique_ptr<Object3d> object3d_;
	std::unique_ptr<Object3d> objectMeleeHand_;
	std::unique_ptr<Object3d> objectLeftHand_;
	std::unique_ptr<Object3d> objectChargeRing_;
	std::unique_ptr<Object3d> objectMeleeFollowUpEffect_;
	EulerTransform meleeHandTransform_{};
	EulerTransform leftHandTransform_{};
	EulerTransform chargeRingTransform_{};
	EulerTransform meleeFollowUpEffectTransform_{};
	bool isMeleeHandVisible_ = false;
	Mode mode_ = Mode::Gameplay;
	bool isMeleeFollowUpEffectActive_ = false;
	float meleeFollowUpEffectTimer_ = 0.0f;
	static constexpr float kMeleeFollowUpEffectDuration_ = 0.18f;
	static constexpr Vector3 kMeleeFollowUpEffectLocalPosition_ =
		{ -1.0f, 0.2f, 1.5f };

	Animation animation_;
	float animationTime = 0.0f;
	Skeleton skeleton_;
	SkinCluster skinCluster_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> skinClusterHeap_;

	std::unique_ptr<Collider> collider_;
	std::unique_ptr<Collider> colliderAttack_;
	bool isAttackColliderActive_ = false;
	static constexpr int kMeleeDamage_ = 5;
	static constexpr int kMeleeFollowUpDamage_ = 3;
	Vector3 meleeAttackDirection_{ 0.0f, 0.0f, 1.0f };
	std::unordered_set<ICollisionObserver*> meleeHitEnemies_;

	EulerTransform transform_;

	Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
	Vector3 maxSpeed_ = { 12.0f, 12.0f, 12.0f }; // スティック全倒し時の最高速度
	float lerpFactor_ = 0.2f;                  // 追従の滑らかさ（0〜1）
	Vector3 targetRoll_ = {0.0f, 0.0f, 0.0f};                   // 目標のロール角Z（回転の傾き）

	// 環境テクスチャのsrvIndex
	uint32_t environmentTextureIndex_ = 0;

	bool isDraw_ = true;

	float bulletSpeed_ = 300.0f;

	// HP
	int hitPoint_;
	// 最大HP
	static inline const int kMaxHitPoint = 25;

	float damageTimer_ = 0.0f;
	const float kDamageInvincible_ = 1.0f;

	// デスフラグ
	bool isDead_ = false;

	bool isHPChanged_ = false;

	EnemyID cachedNearestEnemyID_ = 0;
	Vector3 cachedNearestEnemyPosition_{};
	bool hasNearestEnemy_ = false;
	std::unordered_map<EnemyID, bool> meleeApproachAvailability_;

	bool isLockOnHeld_ = false;
	EnemyID lockOnEnemyID_ = 0;

	DissolveParams dissolveParams_;

	Vector3 avoidDirection_ = {};
	bool justAvoidAccept_ = false;

	std::unique_ptr<ParticleEmitter> justAvoidEmitter_;
	bool isChargeEffectActive_ = false;


	std::unique_ptr<JustAvoidDarken> justAvoidDarken_;
	WorldTransform* parentTransform_ = nullptr;
	Matrix4x4 previousParentWorldMatrix_{};
	Vector3 parentWorldVelocity_{};
	bool hasParentWorldMatrixSample_ = false;
};
