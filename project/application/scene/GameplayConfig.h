#pragma once

#include "myMath.h"

namespace GameplayConfig
{
	inline constexpr Vector3 kPlayerRailLocalPosition{ 0.0f, -1.7f, -7.5f };
	inline constexpr float kBattleSpaceCenterZ = 22.5f;
	inline constexpr float kSkydivingStartHeight = 500.0f;
	inline constexpr float kSkydivingInitialSpeed = 0.0f;
	inline constexpr float kSkydivingAcceleration = 80.0f;
	inline constexpr float kNormalRailSpeed = 300.0f;
	inline constexpr float kCaveEntryHeight = -30.0f;
	inline constexpr float kSkydivingCameraForwardHold = 0.25f;
	inline constexpr float kSkydivingCameraTurnDuration = 0.75f;
	inline constexpr float kCaveFogStartDistance = 20.0f;
	inline constexpr float kCaveFogBaseEndDistance = 500.0f;
}
