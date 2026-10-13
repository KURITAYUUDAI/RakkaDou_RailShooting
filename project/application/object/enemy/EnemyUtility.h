#pragma once
#include <cstdint>

using EnemyID = std::uint32_t;

enum class EnemyType
{
	Boss,
	Stationary,
	Vertical,
	Horizontal,
};

enum class EnemyBattlePhase
{
	MoveToRanged,
	Ranged,
	Approach,
	CloseWait,
	CloseAttack,
	Recovery,
};
