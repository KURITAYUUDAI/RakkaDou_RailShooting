#include "EnemyManager.h"
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
		return enemy->GetIsDead();
	});

	if (!enemies_.empty())
	{
		EnemyID nearestID = FindNearestEnemyID(lastPlayerPosition_);
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
	std::unique_ptr<Enemy> enemy;
	enemy = std::make_unique<Enemy>();
	enemy->Initialize(config);
	enemy->SetEnemyID(nextEnemyID_);
	EnemyID id = nextEnemyID_;
	nextEnemyID_++;
	enemies_.push_back(std::move(enemy));
	return id;
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
	
	//std::vector<float> length;

	//for (auto& enemy : enemies_)
	//{
	//	length.push_back(Length(enemy->GetWorldPosition()));
	//}

	//float maxLength = *max_element(length.begin(), length.end());

	
	auto closest_it = std::min_element(enemies_.begin(), enemies_.end(),
		[&position](const auto& a, const auto& b){
			// targetPos への距離の2乗を計算
			float dist_a_sq = Length(a.get()->GetWorldPosition() - position);

			float dist_b_sq = Length(b.get()->GetWorldPosition() - position);

			return dist_a_sq < dist_b_sq; // 小さい方を優先
		}
	);

	return closest_it->get()->GetEnemyID();
}

void EnemyManager::SetEventBus(EventBus* eventBus)
{
	for (auto& enemy : enemies_)
	{
		enemy->SetEventBus(eventBus);
	}
}
