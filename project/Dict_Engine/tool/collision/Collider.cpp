#define NOMINMAX
#include "Collider.h"
#include "CollisionObserver.h"
#include "DebugDrawManager.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kContactEpsilon = 0.000001f;

	Vector3 SafeNormalize(const Vector3& value)
	{
		return Dot(value, value) > kContactEpsilon
			? Normalize(value)
			: Vector3{ 1.0f, 0.0f, 0.0f };
	}

	CollisionContact MakeSphereContact(
		const Vector3& centerA, float radiusA,
		const Vector3& centerB, float radiusB,
		float timeOfImpact = 1.0f, bool isContinuous = false)
	{
		const Vector3 normal = SafeNormalize(centerB - centerA);
		const Vector3 surfaceA = centerA + normal * radiusA;
		const Vector3 surfaceB = centerB - normal * radiusB;
		return CollisionContact{
			.position = (surfaceA + surfaceB) * 0.5f,
			.normal = normal,
			.timeOfImpact = timeOfImpact,
			.isContinuous = isContinuous,
		};
	}

	CollisionResult CheckContinuousSphereCollision(
		const Collider& a, const Collider& b)
	{
		const Vector3 startA = a.GetPreviousWorldPosition();
		const Vector3 startB = b.GetPreviousWorldPosition();
		const Vector3 movementA = a.GetWorldPosition() - startA;
		const Vector3 movementB = b.GetWorldPosition() - startB;
		const Vector3 relativeStart = startA - startB;
		const Vector3 relativeMovement = movementA - movementB;
		const float radius = a.GetRadius() + b.GetRadius();
		const float quadraticA = Dot(relativeMovement, relativeMovement);
		const float quadraticC = Dot(relativeStart, relativeStart) - radius * radius;

		if (quadraticA <= kContactEpsilon)
		{
			return {};
		}

		float timeOfImpact = 0.0f;
		if (quadraticC > 0.0f)
		{
			const float quadraticB = 2.0f * Dot(relativeStart, relativeMovement);
			const float discriminant =
				quadraticB * quadraticB - 4.0f * quadraticA * quadraticC;
			if (discriminant < 0.0f || !std::isfinite(discriminant))
			{
				return {};
			}

			timeOfImpact =
				(-quadraticB - std::sqrt(discriminant)) / (2.0f * quadraticA);
			if (timeOfImpact < 0.0f || timeOfImpact > 1.0f ||
				!std::isfinite(timeOfImpact))
			{
				return {};
			}
		}

		const Vector3 centerA = startA + movementA * timeOfImpact;
		const Vector3 centerB = startB + movementB * timeOfImpact;
		return CollisionResult{
			.isColliding = true,
			.contact = MakeSphereContact(
				centerA, a.GetRadius(), centerB, b.GetRadius(),
				timeOfImpact, true),
		};
	}

	CollisionContact MakeAABBSphereContact(
		const AABB& box, const Vector3& sphereCenter)
	{
		Vector3 point{
			std::clamp(sphereCenter.x, box.min.x, box.max.x),
			std::clamp(sphereCenter.y, box.min.y, box.max.y),
			std::clamp(sphereCenter.z, box.min.z, box.max.z),
		};
		const Vector3 boxCenter = (box.min + box.max) * 0.5f;
		Vector3 normal = sphereCenter - point;

		if (Dot(normal, normal) <= kContactEpsilon)
		{
			const float distances[6] = {
				sphereCenter.x - box.min.x, box.max.x - sphereCenter.x,
				sphereCenter.y - box.min.y, box.max.y - sphereCenter.y,
				sphereCenter.z - box.min.z, box.max.z - sphereCenter.z,
			};
			const size_t face = static_cast<size_t>(
				std::min_element(distances, distances + 6) - distances);
			switch (face)
			{
			case 0: point.x = box.min.x; break;
			case 1: point.x = box.max.x; break;
			case 2: point.y = box.min.y; break;
			case 3: point.y = box.max.y; break;
			case 4: point.z = box.min.z; break;
			default: point.z = box.max.z; break;
			}
			normal = sphereCenter - boxCenter;
		}

		return CollisionContact{
			.position = point,
			.normal = SafeNormalize(normal),
		};
	}

	CollisionContact MakeAABBContact(const AABB& a, const AABB& b)
	{
		const Vector3 overlapMin{
			std::max(a.min.x, b.min.x),
			std::max(a.min.y, b.min.y),
			std::max(a.min.z, b.min.z),
		};
		const Vector3 overlapMax{
			std::min(a.max.x, b.max.x),
			std::min(a.max.y, b.max.y),
			std::min(a.max.z, b.max.z),
		};
		const Vector3 centerA = (a.min + a.max) * 0.5f;
		const Vector3 centerB = (b.min + b.max) * 0.5f;
		return CollisionContact{
			.position = (overlapMin + overlapMax) * 0.5f,
			.normal = SafeNormalize(centerB - centerA),
		};
	}
}

class IColliderShape
{
public:
	virtual ~IColliderShape() = default;

	virtual ColliderShape GetType() const = 0;
	virtual CollisionResult Collide(
		const Collider& source, const Collider& target) const = 0;
	virtual CollisionResult CollideFromSphere(
		const Collider& source, const Collider& target) const = 0;
	virtual CollisionResult CollideFromAABB(
		const Collider& source, const Collider& target) const = 0;
	virtual void DrawDebug(
		const Collider& collider, const Vector4& color,
		uint32_t sphereSubdivision) const = 0;
};

class SphereColliderShape final : public IColliderShape
{
public:
	ColliderShape GetType() const override { return ColliderShape::Sphere; }

	CollisionResult Collide(
		const Collider& source, const Collider& target) const override
	{
		return target.shape_->CollideFromSphere(source, target);
	}

	CollisionResult CollideFromSphere(
		const Collider& source, const Collider& target) const override
	{
		const Sphere sphereA{ source.GetWorldPosition(), source.GetRadius() };
		const Sphere sphereB{ target.GetWorldPosition(), target.GetRadius() };
		CollisionResult result{};
		result.isColliding = IsCollision(sphereA, sphereB);
		if (result.isColliding)
		{
			result.contact = MakeSphereContact(
				sphereA.center, sphereA.radius, sphereB.center, sphereB.radius);
		}

		if (source.IsContinuousCollisionEnabled() ||
			target.IsContinuousCollisionEnabled())
		{
			const CollisionResult continuousResult =
				CheckContinuousSphereCollision(source, target);
			if (continuousResult.isColliding)
			{
				result = continuousResult;
			}
		}
		return result;
	}

	CollisionResult CollideFromAABB(
		const Collider& source, const Collider& target) const override
	{
		const AABB box = source.GetAABB();
		const Sphere sphere{ target.GetWorldPosition(), target.GetRadius() };
		CollisionResult result{};
		result.isColliding = IsCollision(box, sphere);
		if (result.isColliding)
		{
			result.contact = MakeAABBSphereContact(box, sphere.center);
		}
		return result;
	}

	void DrawDebug(
		const Collider& collider, const Vector4& color,
		uint32_t sphereSubdivision) const override
	{
		DebugDrawManager::GetInstance()->AddSphere(
			collider.GetWorldPosition(), collider.GetRadius(), color,
			sphereSubdivision);
	}
};

class AABBColliderShape final : public IColliderShape
{
public:
	ColliderShape GetType() const override { return ColliderShape::AABB; }

	CollisionResult Collide(
		const Collider& source, const Collider& target) const override
	{
		return target.shape_->CollideFromAABB(source, target);
	}

	CollisionResult CollideFromSphere(
		const Collider& source, const Collider& target) const override
	{
		const Sphere sphere{ source.GetWorldPosition(), source.GetRadius() };
		const AABB box = target.GetAABB();
		CollisionResult result{};
		result.isColliding = IsCollision(box, sphere);
		if (result.isColliding)
		{
			result.contact = MakeAABBSphereContact(box, sphere.center);
			result.contact.normal = result.contact.normal * -1.0f;
		}
		return result;
	}

	CollisionResult CollideFromAABB(
		const Collider& source, const Collider& target) const override
	{
		const AABB boxA = source.GetAABB();
		const AABB boxB = target.GetAABB();
		CollisionResult result{};
		result.isColliding = IsCollision(boxA, boxB);
		if (result.isColliding)
		{
			result.contact = MakeAABBContact(boxA, boxB);
		}
		return result;
	}

	void DrawDebug(
		const Collider& collider, const Vector4& color,
		uint32_t) const override
	{
		DebugDrawManager::GetInstance()->AddBox(
			collider.GetWorldPosition(), collider.GetSize(), color);
	}
};

Collider::Collider()
	: shape_(std::make_unique<SphereColliderShape>())
{
}

Collider::~Collider() = default;

ColliderShape Collider::GetShape() const
{
	return shape_->GetType();
}

void Collider::SetShape(ColliderShape shape)
{
	switch (shape)
	{
	case ColliderShape::Sphere:
		shape_ = std::make_unique<SphereColliderShape>();
		break;
	case ColliderShape::AABB:
		shape_ = std::make_unique<AABBColliderShape>();
		break;
	}
}

CollisionResult Collider::CheckCollision(const Collider& other) const
{
	return shape_->Collide(*this, other);
}

void Collider::DrawDebug(
	const Vector4& color, uint32_t sphereSubdivision) const
{
	shape_->DrawDebug(*this, color, sphereSubdivision);
}

AABB Collider::GetAABB() const
{
	const Vector3 halfSize = size_ * 0.5f;
	return AABB{
		.min = worldPosition_ - halfSize,
		.max = worldPosition_ + halfSize,
	};
}

void Collider::UpdateWorldPosition()
{
	Vector3 position{};
	if (parent_)
	{
		position = TransformPosition(localPosition_, parent_->GetWorldMatrix());
	} 
    else
	{
		position = localPosition_;
	}
	SetWorldPosition(position);
}

void Collider::SetWorldPosition(const Vector3& position)
{
	if (!hasWorldPosition_)
	{
		worldPosition_ = position;
		previousWorldPosition_ = position;
		hasWorldPosition_ = true;
		return;
	}

	previousWorldPosition_ = worldPosition_;
	worldPosition_ = position;
}

void Collider::OnCollision(Collider* other, const CollisionContact& contact)
{
    if (onCollision_)
    {
		onCollision_(this, other, contact);
        return;
    }

    if (owner_) 
    {
        owner_->OnCollision(this, other, contact);
        return;
    }
}
