#pragma once

#include "myMath.h"

struct PlayerHPChangeEvent
{
	int currentHitPoint = 0;
	int previousHitPoint = 1;
	int maxHitPoint = 1;
};

struct PlayerWorldPositionEvent
{
	Vector3 worldPosition{};
};

struct PlayerLockOnEvent
{
	bool isLockOn = false;
	Vector3 targetWorldPosition{};
};

//struct PlayerRockOnEvent
//{
//	EnemyID enemyID;
//	Vector3 enemyPositionEvent{};
//};
