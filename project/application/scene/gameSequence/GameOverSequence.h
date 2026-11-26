#pragma once

#include "GamePlaySequence.h"
#include "PostEffectController.h"
#include "myMath.h"

#include <memory>

class Player;
class RailCameraController;
class Text;

struct GameOverDependencies
{
	Player& player;
	RailCameraController& railCamera;
	PostEffectController& postEffects;
};

struct GameOverConfig
{
	float cameraZoomDistance = 6.0f;
	float cameraCoastDuration = 3.0f;
	float cameraCoastEndSpeedRate = 0.15f;

	float vignetteThreshold = 0.32f;
	float vignetteSoftness = 0.32f;
	float vignetteIntensity = 0.85f;
	float vignetteFadeDuration = 0.5f;

	float hueShift = 0.0f;
	float saturation = 0.15f;
	float value = 0.9f;
	float hsvFadeDuration = 3.0f;

	float textDuration = 2.0f;
};

class GameOverSequence final : public GamePlaySequence
{
public:
	GameOverSequence(
		GameOverDependencies dependencies,
		GameOverConfig config = {});
	~GameOverSequence() override;

	void Start() override;
	SequenceStatus Update(float deltaTime) override;
	SequencePolicy GetPolicy() const override;

	void DrawWorld() override;
	void DrawOverlay() override;
	void DrawUI() override;

private:
	enum class State
	{
		Impact,
		Falling,
		DisplayingText,
		Complete,
	};

	void StartCameraRelease();
	void StartVignette();
	void StartHSVAdjust();
	void CancelPostEffects();
	bool HasReachedAngle(float current, float target) const;

private:
	GameOverDependencies dependencies_;
	GameOverConfig config_;

	State state_ = State::Impact;
	float textTimer_ = 0.0f;
	Vector3 cameraBaseOffset_{};
	std::unique_ptr<Text> gameOverText_;

	PostEffectHandle vignetteHandle_ = kInvalidPostEffectHandle;
	PostEffectHandle hsvHandle_ = kInvalidPostEffectHandle;
};
