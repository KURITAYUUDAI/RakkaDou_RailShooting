#pragma once
#include <cstdint>
#include <vector>
#include "myMath.h"
#include "enemy/EnemyUtility.h"

struct HPViewModel
{
    int currentHitPoint = 0;
    int previousHitPoint = 1;
    int maxHitPoint = 1;
};

enum class EnemyHPGageDisplayType : uint32_t
{
    None = 0,
    ScreenBoss = 1 << 0,
    OverHead = 1 << 1,
};

inline EnemyHPGageDisplayType operator|(
    EnemyHPGageDisplayType a,
    EnemyHPGageDisplayType b)
{
    return static_cast<EnemyHPGageDisplayType>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

inline bool HasEnemyHPGageDisplay(
    EnemyHPGageDisplayType value,
    EnemyHPGageDisplayType flag)
{
    return
        (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

struct EnemyHPViewModel
{
    EnemyID enemyID = 0;

    HPViewModel hitPoint;

    Vector2 screenPosition{};
    Vector3 worldPosition{};
    bool isVisible = true;

    EnemyHPGageDisplayType displayType = EnemyHPGageDisplayType::None;

    int screenBossPriority = 0;
};

struct GameUIViewModel
{
    HPViewModel playerHitPoint;

    Vector3 playerWorldPosition;

    std::vector<EnemyHPViewModel> enemyHitPoints;

    Vector3 targetEnemyWorldPosition;
	EnemyID targetEnemyID = 0;
    bool isLockOn = false;

};
