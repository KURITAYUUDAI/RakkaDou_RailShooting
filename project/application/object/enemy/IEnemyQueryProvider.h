#pragma once

#include "EnemyUtility.h"
#include "myMath.h"

struct EnemyTargetInfo
{
	EnemyID id = 0;
	Vector3 worldPosition{};
	Vector3 worldVelocity{};
	bool canReceiveMeleeApproach = false;
};

class IEnemyQueryProvider
{
public:
	virtual ~IEnemyQueryProvider() = default;

	virtual bool TryGetEnemyTarget(
		EnemyID id, EnemyTargetInfo& outTarget) const = 0;
	virtual bool TryGetNearestEnemyTarget(
		const Vector3& origin, EnemyTargetInfo& outTarget) const = 0;
};
