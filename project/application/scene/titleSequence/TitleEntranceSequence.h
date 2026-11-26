#pragma once

#include "TitleSequence.h"
#include "myMath.h"

class Camera;
class DefaultCameraController;
class Player;
class Sprite;

struct TitleEntranceDependencies
{
	Player& player;
	Camera& camera;
	DefaultCameraController& cameraController;
	Sprite& titleSprite;
	Sprite& openingFadeSprite;
};

struct TitleEntranceConfig
{
	Vector3 playerStartPosition{};
	Vector3 playerFinalPosition{};
	Vector3 cameraStartPosition = { 0.0f, 15.0f, -12.0f };
	Vector3 cameraSidePosition = { 0.0f, 0.0f, -12.0f };
	Vector3 cameraFinalPosition = { 0.0f, 20.0f, 0.0f };
	Vector3 cameraSideRotation = { 0.0f, 0.0f, 0.0f };
	Vector3 cameraFinalRotation = { 1.57079632679f, 0.0f, 0.0f };

	float fallDuration = 4.0f;
	float cameraDelay = 1.5f;
	float cameraMoveDuration = 0.5f;
	float titleFadeDuration = 1.5f;
	float openingFadeDuration = 0.75f;
};

class TitleEntranceSequence final : public TitleSequence
{
public:
	TitleEntranceSequence(
		TitleEntranceDependencies dependencies,
		TitleEntranceConfig config = {});

	void Start() override;
	TitleSequenceStatus Update(float deltaTime) override;
	bool HandleConfirm() override;
	TitleSequenceCompletionAction GetCompletionAction() const override;
	void DrawOverlay(bool drawTitle) override;
	bool IsComplete() const;
	TitleEntranceConfig& GetConfig() { return config_; }
	const TitleEntranceConfig& GetConfig() const { return config_; }

private:
	enum class State
	{
		PlayerFalling,
		WaitingForCamera,
		MovingCamera,
		FadingTitle,
		Complete,
	};

	void SetCameraTransform(
		const Vector3& translate,
		const Vector3& rotate);
	void Complete();

private:
	TitleEntranceDependencies dependencies_;
	TitleEntranceConfig config_;
	State state_ = State::PlayerFalling;
	float stateTimer_ = 0.0f;
};
