#pragma once

#include "GamePlaySequence.h"
#include "PostEffectController.h"
#include "myMath.h"

#include <array>
#include <memory>

class CameraManager;
class DefaultCameraController;
class Object3d;
class Player;
class RailCameraController;
class Sprite;

struct StageEntryDependencies
{
	Player& player;
	CameraManager& cameraManager;
	DefaultCameraController& defaultCamera;
	RailCameraController& railCamera;
	PostEffectController& postEffects;
};

struct StageEntryConfig
{
	Vector3 doorPosition = { 0.0f, 3.2f, 8.0f };
	Vector3 playerStartPosition = { 0.0f, -1.7f, -7.0f };
	Vector3 playerDoorPosition = { 0.0f, 0.0f, 1.5f };
	Vector3 playerDiveEndPosition = { 0.0f, 0.0f, 10.5f };
	Vector3 gameplayPlayerRailPosition = { 0.0f, -1.7f, -7.5f };

	float skydivingStartHeight = 500.0f;
	float skydivingInitialSpeed = 20.0f;
	float normalRailSpeed = 300.0f;

	float blackHoldDuration = 0.25f;
	float approachDuration = 0.90f;
	float doorOpenDuration = 0.65f;
	float diveDuration = 0.55f;
	float whiteOutDuration = 0.20f;
	float whiteFadeDuration = 0.45f;

	float lookDownForwardHoldDuration = 0.25f;
	float lookDownTurnDuration = 0.75f;
	float blurMaxWidth = 0.02f;
};

class StageEntrySequence final : public GamePlaySequence
{
public:
	StageEntrySequence(
		StageEntryDependencies dependencies,
		StageEntryConfig config = {});
	~StageEntrySequence() override;

	void Start() override;
	SequenceStatus Update(float deltaTime) override;
	SequenceCompletionAction GetCompletionAction() const override;
	SequencePolicy GetPolicy() const override;

	void DrawWorld() override;
	void DrawOverlay() override;
	void DrawUI() override;

	void OnFadeInFinished() override;

private:
	enum class State
	{
		WaitingForFadeIn,
		BlackHold,
		Approaching,
		OpeningDoor,
		Diving,
		WhiteOut,
		LookDown,
		Complete,
	};

	void InitializeAssets();
	void BeginLookDown();
	void UpdateLookDownCamera();
	void UpdateEntryObjects();

	void StartBlur();
	void UpdateBlur(float progress);
	void StopBlur();

private:
	StageEntryDependencies dependencies_;
	StageEntryConfig config_;

	State state_ = State::WaitingForFadeIn;
	float stateTimer_ = 0.0f;
	float doorProgress_ = 0.0f;
	float lookDownCameraTurnTimer_ = 0.0f;
	bool assetsInitialized_ = false;

	std::unique_ptr<Object3d> backdrop_;
	std::unique_ptr<Object3d> light_;
	std::unique_ptr<Object3d> leftDoor_;
	std::unique_ptr<Object3d> rightDoor_;
	std::array<std::unique_ptr<Object3d>, 3> doorFrames_;
	std::unique_ptr<Sprite> whiteOverlay_;

	PostEffectHandle blurHandle_ = kInvalidPostEffectHandle;
};
