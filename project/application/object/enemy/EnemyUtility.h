#pragma once
#include <cstdint>

using EnemyID = std::uint32_t;

enum class EnemyType
{
	Boss,
	Small,
};

enum class SmallEnemyRole
{
	Basic,
	Turret,
	Charger,
	Approacher,
	Elite,
};

enum class SmallEnemyState
{
	Entering,
	SpawnWait,
	Ranged,
	Charging,
	Rushing,
	Approaching,
	NearCombat,
	Escaping,
};

inline const char* ToString(SmallEnemyRole role)
{
	switch (role)
	{
	case SmallEnemyRole::Basic: return "Basic";
	case SmallEnemyRole::Turret: return "Turret";
	case SmallEnemyRole::Charger: return "Charger";
	case SmallEnemyRole::Approacher: return "Approacher";
	case SmallEnemyRole::Elite: return "Elite";
	}
	return "Unknown";
}

inline const char* ToString(SmallEnemyState state)
{
	switch (state)
	{
	case SmallEnemyState::Entering: return "Entering";
	case SmallEnemyState::SpawnWait: return "SpawnWait";
	case SmallEnemyState::Ranged: return "Ranged";
	case SmallEnemyState::Charging: return "Charging";
	case SmallEnemyState::Rushing: return "Rushing";
	case SmallEnemyState::Approaching: return "Approaching";
	case SmallEnemyState::NearCombat: return "NearCombat";
	case SmallEnemyState::Escaping: return "Escaping";
	}
	return "Unknown";
}

enum class EnemyBattlePhase
{
	MoveToRanged,
	Ranged,
	Approach,
	CloseWait,
	CloseAttack,
	Recovery,
};
