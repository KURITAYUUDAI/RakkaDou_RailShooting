#pragma once

class CaveGameplayEnvironment;
class CaveRailPath;
class RailCameraController;

struct StageProgressLineConfig
{
	float skydivingStartHeight = 500.0f;
	float initialSpeed = 20.0f;
	float acceleration = 80.0f;
	float caveEnteringDuration = 0.35f;
	float normalRailSpeed = 300.0f;
};

struct StageProgressLineDependencies
{
	RailCameraController& railCamera;
	CaveRailPath& railPath;
	CaveGameplayEnvironment& environment;
};

class StageProgressLine
{
public:
	StageProgressLine(
		StageProgressLineDependencies dependencies,
		StageProgressLineConfig config = {});

	void Reset();
	void StartSkydiving();
	bool Update(float deltaTime, float playerHeight);

	bool IsActive() const;
	bool IsEntryFieldVisible() const;
	bool IsOnRail() const;
	float GetCurrentSpeed() const;
	const StageProgressLineConfig& GetConfig() const;

private:
	enum class State
	{
		Inactive,
		Skydiving,
		CaveEntering,
		Playing,
	};

	void UpdateSpeed(float deltaTime);
	void BeginCaveEntering(float playerHeight);
	void BeginPlaying();

private:
	StageProgressLineDependencies dependencies_;
	StageProgressLineConfig config_;
	State state_ = State::Inactive;
	float stateTimer_ = 0.0f;
	float currentSpeed_ = 0.0f;
};
