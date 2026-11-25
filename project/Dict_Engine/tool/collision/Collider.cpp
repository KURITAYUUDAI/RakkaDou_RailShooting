#include "Collider.h"
#include "CollisionObserver.h"

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
