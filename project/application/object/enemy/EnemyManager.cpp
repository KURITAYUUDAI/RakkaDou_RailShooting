#include "EnemyManager.h"
#include "BossEnemy.h"
#include "SmallEnemy.h"
#include "time/DeltaTimeManager.h"

std::unique_ptr<EnemyManager> EnemyManager::instance_ = nullptr;

EnemyManager* EnemyManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<EnemyManager>(ConstructorKey());
	}
	return instance_.get();
}

void EnemyManager::Finalize()
{
	enemies_.clear();
	pooledEnemies_.clear();
	instance_.reset();
}

void EnemyManager::Initialize()
{
	RegisterPSO();
}

void EnemyManager::Update(
	const float& deltaTime, const EnemyUpdateContext& context)
{
	(void)deltaTime;
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Enemy);

	for (auto& enemy : enemies_)
	{
		enemy->SetRailFollowContext(railFollowContext_);
		enemy->Update(deltaTime_, context);
	}

	enemies_.remove_if([](const std::unique_ptr<Enemy>& enemy){
		return enemy->GetIsRemove();
	});

}

void EnemyManager::Draw()
{
	for (auto& enemy : enemies_)
	{
		enemy->Draw();
	}
}

void EnemyManager::RegisterPSO()
{
	PSOManager::GetInstance()->RegisterEnvironmentPSO();
}

EnemyID EnemyManager::AddEnemy(const EnemyCreateConfig& config)
{
	std::unique_ptr<Enemy> enemy = CreateEnemy(config);
	enemy->SetRailFollowContext(railFollowContext_);
	enemy->SetEnemyID(nextEnemyID_);
	const EnemyID id = nextEnemyID_;
	++nextEnemyID_;
	enemies_.push_back(std::move(enemy));
	return id;
}

EnemyManager::PoolHandle EnemyManager::PrewarmEnemy(const EnemyCreateConfig& config)
{
	PooledEnemy pooledEnemy;
	pooledEnemy.handle = nextPoolHandle_++;
	if (nextPoolHandle_ == kInvalidPoolHandle)
	{
		++nextPoolHandle_;
	}
	pooledEnemy.enemy = CreateEnemy(config);
	const PoolHandle handle = pooledEnemy.handle;
	pooledEnemies_.push_back(std::move(pooledEnemy));
	return handle;
}

EnemyID EnemyManager::ActivatePrewarmedEnemy(PoolHandle handle)
{
	if (handle == kInvalidPoolHandle)
	{
		return 0;
	}

	for (auto iterator = pooledEnemies_.begin();
		iterator != pooledEnemies_.end(); ++iterator)
	{
		if (iterator->handle != handle)
		{
			continue;
		}

		std::unique_ptr<Enemy> enemy = std::move(iterator->enemy);
		pooledEnemies_.erase(iterator);
		enemy->SetRailFollowContext(railFollowContext_);
		enemy->SetEnemyID(nextEnemyID_);
		const EnemyID id = nextEnemyID_;
		++nextEnemyID_;
		enemies_.push_back(std::move(enemy));
		return id;
	}

	return 0;
}

void EnemyManager::SetRailFollowContext(
	const CaveRailPath* path, float viewerDistance,
	const Vector3& pathWorldOffset)
{
	railFollowContext_.path = path;
	railFollowContext_.viewerDistance = viewerDistance;
	railFollowContext_.pathWorldOffset = pathWorldOffset;
	for (auto& enemy : enemies_)
	{
		enemy->SetRailFollowContext(railFollowContext_);
	}
}

std::unique_ptr<Enemy> EnemyManager::CreateEnemy(const EnemyCreateConfig& config)
{
	std::unique_ptr<Enemy> enemy;
	if (config.type == EnemyType::Boss)
	{
		enemy = std::make_unique<BossEnemy>();
	}
	else
	{
		enemy = std::make_unique<SmallEnemy>();
	}
	enemy->Initialize(config);
	return enemy;
}

void EnemyManager::RemoveSmallEnemies()
{
	enemies_.remove_if([](const std::unique_ptr<Enemy>& enemy)
		{
			return !enemy->GetIsBoss();
		});
}

std::vector<EnemyID> EnemyManager::GetEnemyIDs() const
{
	std::vector<EnemyID> ids;
	ids.reserve(enemies_.size());
	for (const auto& enemy : enemies_)
	{
		ids.push_back(enemy->GetEnemyID());
	}
	return ids;
}

Enemy* EnemyManager::FindEnemy(EnemyID id)
{
	for (auto& enemy : enemies_)
	{
		if (enemy->GetEnemyID() == id)
		{
			return enemy.get();
		}
	}

	return nullptr;
}

const Enemy* EnemyManager::FindEnemy(EnemyID id) const
{
	for (auto& enemy : enemies_)
	{
		if (enemy->GetEnemyID() == id)
		{
			return enemy.get();
		}
	}

	return nullptr;
}

bool EnemyManager::GetEnemyPosition(EnemyID id, Vector3& outPosition) const
{
	for (auto& enemy : enemies_)
	{
		if (enemy->GetEnemyID() == id)
		{
			outPosition = enemy->GetWorldPosition();
			return true;
		}
	}

	return false;
}

bool EnemyManager::GetIsDead(EnemyID id) const
{
	for (auto& enemy : enemies_)
	{
		if (enemy->GetEnemyID() == id)
		{
			return enemy->GetIsDead();
		}
	}

	return true;
}

EnemyID EnemyManager::FindNearestEnemyID(const Vector3& position) const
{
	const Enemy* nearestEnemy = nullptr;
	float nearestDistance = 0.0f;
	for (const auto& enemy : enemies_)
	{
		if (enemy->GetIsDead())
		{
			continue;
		}

		const float distance = Length(enemy->GetWorldPosition() - position);
		if (!nearestEnemy || distance < nearestDistance)
		{
			nearestEnemy = enemy.get();
			nearestDistance = distance;
		}
	}

	return nearestEnemy ? nearestEnemy->GetEnemyID() : 0;
}

bool EnemyManager::TryGetEnemyTarget(
	EnemyID id, EnemyTargetInfo& outTarget) const
{
	outTarget = {};
	const Enemy* enemy = FindEnemy(id);
	if (!enemy || enemy->GetIsDead())
	{
		return false;
	}

	outTarget.id = enemy->GetEnemyID();
	outTarget.worldPosition = enemy->GetWorldPosition();
	outTarget.worldVelocity = enemy->GetWorldVelocity();
	outTarget.canReceiveMeleeApproach = enemy->CanReceiveMeleeApproach();
	return true;
}

bool EnemyManager::TryGetNearestEnemyTarget(
	const Vector3& origin, EnemyTargetInfo& outTarget) const
{
	const EnemyID nearestID = FindNearestEnemyID(origin);
	return TryGetEnemyTarget(nearestID, outTarget);
}

