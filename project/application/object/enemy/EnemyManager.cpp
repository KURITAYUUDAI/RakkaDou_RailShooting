#include "EnemyManager.h"
#include "BossEnemy.h"
#include "SmallEnemy.h"
#include "PlayerEvent.h"
#include "EnemyEvent.h"
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
	eventSubscriber_.Finalize();
	enemies_.clear();
	pooledEnemies_.clear();
	instance_.reset();
}

void EnemyManager::Initialize(EventBus* eventBus)
{
	RegisterPSO();

	eventSubscriber_.Initialize(eventBus);

	eventSubscriber_.Subscribe<PlayerWorldPositionEvent>(
		[this](const PlayerWorldPositionEvent& event)
		{
			lastPlayerPosition_ = event.worldPosition;
		}
	);

	/*eventSubscriber_.GetEventBus()->Publish(NearestEnemyWorldPositionEvent
		{
			.worldPosition = GetWorldPosition(),
		}
	);*/
}

void EnemyManager::Update(const float& deltaTime)
{
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Enemy);

	for (auto& enemy : enemies_)
	{
		enemy->Update(deltaTime_);
	}

	enemies_.remove_if([](const std::unique_ptr<Enemy>& enemy){
		return enemy->IsReadyForRemoval();
	});

	const EnemyID nearestID = FindNearestEnemyID(lastPlayerPosition_);
	if (nearestID != 0)
	{
		Vector3 nearestPos{};
		GetEnemyPosition(nearestID, nearestPos);

		eventSubscriber_.GetEventBus()->Publish(NearestEnemyInfoEvent
			{
				.enemyID = nearestID,
				.worldPosition = nearestPos,
				.isValid = true
			}
		);
	} else
	{
		eventSubscriber_.GetEventBus()->Publish(NearestEnemyInfoEvent
			{
				.enemyID = 0,
				.worldPosition = {},
				.isValid = false
			}
		);
	}
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
		enemy->SetEnemyID(nextEnemyID_);
		const EnemyID id = nextEnemyID_;
		++nextEnemyID_;
		enemies_.push_back(std::move(enemy));
		return id;
	}

	return 0;
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

void EnemyManager::SetEventBus(EventBus* eventBus)
{
	for (auto& enemy : enemies_)
	{
		enemy->SetEventBus(eventBus);
	}
}
