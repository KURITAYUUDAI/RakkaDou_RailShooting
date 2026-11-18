#pragma once
#include "EnemyUtility.h"
#include "myMath.h"

struct EnemyAppearedEvent
{
    EnemyID enemyID = 0;
    EnemyType enemyType = EnemyType::Small;
    Vector3 worldPosition{};
};

struct EnemyHPChangeEvent
{
    EnemyID enemyID = 0;
    int currentHitPoint = 0;
    int previousHitPoint = 1;
    int maxHitPoint = 1;
};

struct BossDefeatedSequenceStartedEvent
{
    EnemyID enemyID = 0;
};

struct BossDefeatedSequenceFinishedEvent
{
    EnemyID enemyID = 0;
};
