#pragma once
#include "myMath.h"
#include "object3d.h"
#include "WorldTransform.h"
#include <memory>

#include "collision/CollisionObserver.h"
#include "collision/Collider.h"

class Bullet : public ICollisionObserver
{
public:

	enum class ID
	{
		kPlayer,
		kEnemy,
	};

public:
	/// <summary>
	/// 弾を初期化する。parent指定時はpositionとvelocityを親のローカル座標として扱う。
	/// </summary>
	void Initialize(
		const Vector3& position, const Vector3& velocity, const ID& id,
		const std::string& modelName, WorldTransform* parent = nullptr);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(const float& deltaTime);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	void Finalize();

	/*void OnCollision(const Enemy* enemy);*/

	/*void OnCollision(const Player* player);*/

	void OnCollision(
		Collider* self, Collider* other, const CollisionContact& contact) override;

public: // 外部入出力
	const Vector3& GetScale() { return transform_.scale; }
	const Vector3& GetRotation() { return transform_.rotate; }
	const Vector3& GetTranslation() { return transform_.translate; }
	const Vector3& GetVelocity() { return velocity_; }
	const int32_t& GetDeathTimer(){ return deathTimer_; }
	const bool GetIsDead() { return isDead_; }
	const ID& GetID() { return id_; }

	// ワールドポジション
	const Vector3 GetWorldPosition() const;
	// AABB
	AABB GetAABB();

	Collider* GetCollider() { return collider_.get(); }

	void SetRotation(const Vector3& rotation) { transform_.rotate = rotation; }
	void SetSize(const Vector3& size) { size_ = size; }
	void SetTranslation(const Vector3& translation) { transform_.translate = translation; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

	void SetPsoName(const std::string& psoName){ object3d_->SetPsoName(psoName); }

private:
	void UpdateObjectAndCollider();

	std::unique_ptr<Object3d> object3d_;
	std::unique_ptr<Collider> collider_;
	WorldTransform* parent_ = nullptr;

	EulerTransform transform_;

	Vector3 size_ = { 0.4f, 0.4f, 0.4f };

	const float speed_ = 2.0f;

	Vector3 direction_;

	Vector3 velocity_;

	ID id_;

	// 寿命
	static inline const int32_t kLifeTime = 60 * 5;

	// デスタイマー
	int32_t deathTimer_ = kLifeTime;
	// デスフラグ
	bool isDead_ = false;

	bool isJustAvoid_ = false;
};

