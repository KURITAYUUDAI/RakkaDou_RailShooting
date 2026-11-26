#pragma once

#include "EnemyManager.h"
#include "EnemyStageConfig.h"

#include <string>
#include <vector>

class EventBus;
class RailCameraController;
class SkyBox;
class Terrain;

struct EnemyStageDependencies
{
	EnemyManager& enemyManager;
	RailCameraController& railCamera;
	Terrain& terrain;
	SkyBox& skyBox;
	EventBus& eventBus;
};

struct EnemyStageDirectorConfig
{
	std::string stageConfigPath = "enemy/enemy_stage.json";
	float spawnDelay = 4.0f;
};

class EnemyStageDirector
{
public:
	EnemyStageDirector(
		EnemyStageDependencies dependencies,
		EnemyStageDirectorConfig config = {});

	void Initialize();
	void Update(float deltaTime);
	void SkipToBossPhase();
	void DrawDebugUI() const;

	bool IsCurrentBoss(EnemyID enemyID) const;
	bool IsBossPhase() const;

private:
	enum class Phase
	{
		SmallEnemies,
		Boss,
	};

	void SpawnSmallEnemy(
		const SmallEnemySpawnConfig& spawnConfig,
		size_t spawnIndex);
	void StartBossPhase();
	void ConfigureEnemy(EnemyID id, bool isBoss);
	float GetBossPhaseDistance() const;

private:
	EnemyStageDependencies dependencies_;
	EnemyStageDirectorConfig directorConfig_;
	EnemyStageConfig stageConfig_;
	Phase phase_ = Phase::SmallEnemies;
	size_t nextSmallEnemyIndex_ = 0;
	EnemyID bossEnemyID_ = 0;
	std::vector<EnemyManager::PoolHandle> smallEnemyPoolHandles_;
	EnemyManager::PoolHandle bossEnemyPoolHandle_ =
		EnemyManager::kInvalidPoolHandle;
};
