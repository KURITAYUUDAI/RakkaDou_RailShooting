#pragma once

struct PlayerHPChangeEvent
{
	int currentHitPoint = 0;
	int previousHitPoint = 1;
	int maxHitPoint = 1;
};
