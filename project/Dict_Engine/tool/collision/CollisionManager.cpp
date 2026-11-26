#define NOMINMAX
#include "CollisionManager.h"
#include "Logger.h"

std::unique_ptr<CollisionManager> CollisionManager::instance_ = nullptr;

CollisionManager* CollisionManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<CollisionManager>(ConstructorKey());
	}
	return instance_.get();
}

void CollisionManager::Finalize()
{
	Clear();
	instance_.reset();
}

void CollisionManager::AddCollider(Collider* collider)
{
	colliders_.push_back(collider);
}

void CollisionManager::Clear()
{
	colliders_.clear();
}

void CollisionManager::CheckAllCollisions()
{
    for (auto itrA = colliders_.begin(); itrA != colliders_.end(); ++itrA) 
    {
        auto itrB = itrA;
        ++itrB;

        for (; itrB != colliders_.end(); ++itrB) 
        {
            Collider* a = *itrA;
            Collider* b = *itrB;

			if ((a->GetAttribute() & b->GetMask()) == 0 ||
				(b->GetAttribute() & a->GetMask()) == 0)
			{
				continue;
			}

			const CollisionResult result = a->CheckCollision(*b);

			if (result.isColliding)
            {
				a->OnCollision(b, result.contact);
				CollisionContact inverseContact = result.contact;
				inverseContact.normal = inverseContact.normal * -1.0f;
				b->OnCollision(a, inverseContact);
            }
        }
    }
}

