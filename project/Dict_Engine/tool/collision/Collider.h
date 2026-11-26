#pragma once
#include "myMath.h"
#include <functional>
#include <memory>
#include "WorldTransform.h"

class ICollisionObserver;
class IColliderShape;
class SphereColliderShape;
class AABBColliderShape;

struct CollisionContact
{
	Vector3 position{};
	// このColliderから衝突相手へ向かうワールド空間の法線。
	Vector3 normal{ 1.0f, 0.0f, 0.0f };
	// 1.0は現在フレーム終端。Sweep時は[0, 1]内の最初の接触時刻。
	float timeOfImpact = 1.0f;
	bool isContinuous = false;
};

struct CollisionResult
{
	bool isColliding = false;
	CollisionContact contact{};
};

enum class CollisionAttribute : uint32_t
{
	None = 0,
	Player = 0b1,
	PlayerAttack = 0b1 << 1,
	Enemy = 0b1 << 2,
	EnemyAttack = 0b1 << 3,
};

enum class ColliderShape
{
	Sphere,
	AABB,
};

class Collider
{
public:

	using CollisionCallback =
		std::function<void(Collider*, Collider*, const CollisionContact&)>;

public:
	Collider();
	~Collider();

	const Vector3& GetWorldPosition() const { return worldPosition_; }
	const Vector3& GetPreviousWorldPosition() const { return previousWorldPosition_; }
	const float& GetRadius() const { return radius_; }
	ColliderShape GetShape() const;
	const Vector3& GetSize() const { return size_; }
	AABB GetAABB() const;
	CollisionResult CheckCollision(const Collider& other) const;
	void DrawDebug(const Vector4& color, uint32_t sphereSubdivision = 8) const;
	uint32_t GetAttribute() const { return attribute_; }
	uint32_t GetMask() const {return mask_; }
	int GetDamage() const { return damage_; }
	bool IsContinuousCollisionEnabled() const { return continuousCollisionEnabled_; }

	ICollisionObserver* GetOwner() { return owner_; }
	
	void SetParent(WorldTransform* parent) { parent_ = parent; }
	void SetLocalPosition(const Vector3& localPosition) { localPosition_ = localPosition; }
	void UpdateWorldPosition();
	
	void SetWorldPosition(const Vector3& position);

	void SetRadius(const float& radius){ radius_ = radius; }
	void SetShape(ColliderShape shape);
	void SetSize(const Vector3& size) { size_ = size; }

	void SetAttribute(CollisionAttribute attribute) { attribute_ = static_cast<uint32_t>(attribute); }
	void SetAttribute(uint32_t attribute){ attribute_ = attribute; }
	void SetMask(CollisionAttribute mask) { mask_ = ~static_cast<uint32_t>(mask); }
	void SetMask(uint32_t mask) { mask_ = mask; }
	void SetDamage(int damage) { damage_ = damage; }
	void SetContinuousCollisionEnabled(bool enabled) { continuousCollisionEnabled_ = enabled; }

	void SetOwner(ICollisionObserver* owner) { owner_ = owner; }
	void SetOnCollision(CollisionCallback callback) { onCollision_ = std::move(callback); }
	
	void OnCollision(Collider* other, const CollisionContact& contact);

private:
	friend class SphereColliderShape;
	friend class AABBColliderShape;

	float radius_ = 1.0f;
	Vector3 size_{ 2.0f, 2.0f, 2.0f };
	std::unique_ptr<IColliderShape> shape_;
	ICollisionObserver* owner_ = nullptr;
	CollisionCallback onCollision_;

	uint32_t attribute_ = 0xffffffff;
	uint32_t mask_ = 0xffffffff;
	int damage_ = 0;



	WorldTransform* parent_ = nullptr;
	Vector3 localPosition_{};
	Vector3 worldPosition_{};
	Vector3 previousWorldPosition_{};
	bool hasWorldPosition_ = false;
	bool continuousCollisionEnabled_ = false;
};
