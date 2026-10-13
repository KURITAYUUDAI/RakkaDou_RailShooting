#pragma once

#include "Enemy.h"

#include <string>
#include <vector>

struct SmallEnemySpawnConfig
{
	float railRatio = 0.0f;
	EnemyCreateConfig enemy;
};

struct EnemyStageConfig
{
	float smallEnemyPhaseDuration = 35.0f;
	std::vector<SmallEnemySpawnConfig> smallEnemies;
	EnemyCreateConfig boss;

	static EnemyStageConfig CreateDefault();
	bool Load(const std::string& relativePath);
};
