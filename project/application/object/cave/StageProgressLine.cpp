#define NOMINMAX
#include "StageProgressLine.h"

#include "CaveGameplayEnvironment.h"
#include "CaveRailPath.h"
#include "RailCameraController.h"

#include <algorithm>
#include <utility>

StageProgressLine::StageProgressLine(
	StageProgressLineDependencies dependencies,
	StageProgressLineConfig config)
	: dependencies_(dependencies),
	  config_(std::move(config))
{
}

void StageProgressLine::Reset()
{
	state_ = State::Inactive;
	stateTimer_ = 0.0f;
	currentSpeed_ = 0.0f;
}

void StageProgressLine::StartSkydiving()
{
	state_ = State::Skydiving;
	stateTimer_ = 0.0f;
	currentSpeed_ = std::clamp(
		config_.initialSpeed, 0.0f, config_.normalRailSpeed);
	dependencies_.railCamera.SetMoveSpeed(currentSpeed_);
	dependencies_.railCamera.SetMovementEnabled(true);
	dependencies_.environment.SetEntryMode(true);
}

bool StageProgressLine::Update(
	float deltaTime, float playerHeight)
{
	switch (state_)
	{
	case State::Inactive:
	case State::Playing:
		return false;

	case State::Skydiving:
		UpdateSpeed(deltaTime);
		dependencies_.environment.UpdateFog(playerHeight);
		if (dependencies_.railPath.IsUnderground(
			dependencies_.railCamera.GetCurrentDistance()))
		{
			BeginCaveEntering(playerHeight);
		}
		return false;

	case State::CaveEntering:
		UpdateSpeed(deltaTime);
		dependencies_.environment.UpdateFog(playerHeight);
		stateTimer_ += deltaTime;
		if (stateTimer_ >= config_.caveEnteringDuration)
		{
			BeginPlaying();
			return true;
		}
		return false;
	}

	return false;
}

bool StageProgressLine::IsActive() const
{
	return state_ != State::Inactive;
}

bool StageProgressLine::IsEntryFieldVisible() const
{
	return state_ == State::Skydiving ||
		state_ == State::CaveEntering;
}

bool StageProgressLine::IsOnRail() const
{
	return state_ == State::CaveEntering || state_ == State::Playing;
}

float StageProgressLine::GetCurrentSpeed() const
{
	return currentSpeed_;
}

const StageProgressLineConfig& StageProgressLine::GetConfig() const
{
	return config_;
}

void StageProgressLine::UpdateSpeed(float deltaTime)
{
	currentSpeed_ += config_.acceleration * deltaTime;
	currentSpeed_ = std::clamp(
		currentSpeed_, 0.0f, config_.normalRailSpeed);
	dependencies_.railCamera.SetMoveSpeed(currentSpeed_);
}

void StageProgressLine::BeginCaveEntering(float playerHeight)
{
	dependencies_.environment.UpdateFog(playerHeight);
	dependencies_.environment.SetEntryMode(false);
	stateTimer_ = 0.0f;
	state_ = State::CaveEntering;
}

void StageProgressLine::BeginPlaying()
{
	stateTimer_ = 0.0f;
	state_ = State::Playing;
	currentSpeed_ = config_.normalRailSpeed;
	dependencies_.railCamera.SetMoveSpeed(config_.normalRailSpeed);
}
