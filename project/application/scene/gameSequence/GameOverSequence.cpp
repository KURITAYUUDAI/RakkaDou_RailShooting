#define NOMINMAX
#include "GameOverSequence.h"

#include "HSVAdjust.h"
#include "Player.h"
#include "RailCameraController.h"
#include "Text.h"
#include "Vignette.h"

#include <algorithm>
#include <cmath>

namespace
{
	float SmoothStep01(float value)
	{
		const float t = std::clamp(value, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
}

GameOverSequence::GameOverSequence(
	GameOverDependencies dependencies,
	GameOverConfig config)
	: dependencies_(dependencies),
	  config_(config)
{
}

GameOverSequence::~GameOverSequence()
{
	CancelPostEffects();
	dependencies_.railCamera.SetOffset(cameraBaseOffset_);

	if (gameOverText_)
	{
		gameOverText_->Finalize();
	}
}

void GameOverSequence::Start()
{
	state_ = State::Impact;
	textTimer_ = 0.0f;
	cameraBaseOffset_ = dependencies_.railCamera.GetOffset();

	gameOverText_ = std::make_unique<Text>();
	gameOverText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf", 64, U"GAME OVER");
	gameOverText_->SetAnchorPoint({ 0.5f, 0.5f });
	gameOverText_->SetPosition({ 640.0f, 240.0f });
	gameOverText_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
	gameOverText_->Update();

	dependencies_.player.StartGameOverSequence();
	StartVignette();
}

SequenceStatus GameOverSequence::Update(float deltaTime)
{
	if (state_ == State::Complete)
	{
		return SequenceStatus::Finished;
	}

	if (state_ == State::Impact)
	{
		const float recoilAngle = dependencies_.player.GetGameOverRecoilAngle();
		const float impactAngle = dependencies_.player.GetGameOverImpactAngle();
		float zoomRate = 1.0f;
		if (!HasReachedAngle(recoilAngle, impactAngle) &&
			std::abs(impactAngle) > 0.0001f)
		{
			zoomRate = SmoothStep01(std::clamp(
				recoilAngle / impactAngle, 0.0f, 1.0f));
		}

		Vector3 offset = cameraBaseOffset_;
		offset.z += config_.cameraZoomDistance * zoomRate;
		dependencies_.railCamera.SetOffset(offset);

		if (HasReachedAngle(
			recoilAngle,
			dependencies_.player.GetGameOverCameraReleaseAngle()))
		{
			StartCameraRelease();
			state_ = State::Falling;
		}
	}

	if (state_ == State::Falling && dependencies_.player.GetIsRemove())
	{
		state_ = State::DisplayingText;
		textTimer_ = 0.0f;
	}

	if (state_ == State::DisplayingText)
	{
		textTimer_ += deltaTime;
		gameOverText_->Update();
		if (textTimer_ >= config_.textDuration)
		{
			state_ = State::Complete;
			return SequenceStatus::Finished;
		}
	}

	return SequenceStatus::Running;
}

SequencePolicy GameOverSequence::GetPolicy() const
{
	SequencePolicy policy{};
	policy.world = WorldPresentation::GameplayWorld;
	policy.simulation = GameplaySimulation::Running;
	policy.playerControl = PlayerControl::Sequence;
	policy.playerVisibility = PlayerVisibility::Visible;
	policy.gameplayActors = GameplayActorVisibility::Visible;
	policy.hud = HudVisibility::Hidden;
	policy.pauseEnabled = false;
	return policy;
}

void GameOverSequence::DrawWorld()
{
}

void GameOverSequence::DrawOverlay()
{
}

void GameOverSequence::DrawUI()
{
	if (state_ == State::DisplayingText && gameOverText_)
	{
		gameOverText_->Draw();
	}
}

void GameOverSequence::StartCameraRelease()
{
	dependencies_.railCamera.SetMovementEnabled(true);
	dependencies_.railCamera.StartCameraCoast(
		config_.cameraCoastDuration,
		config_.cameraCoastEndSpeedRate);
	StartHSVAdjust();
}

void GameOverSequence::StartVignette()
{
	vignetteHandle_ = dependencies_.postEffects.Emit<Vignette>(
		"Vignette", std::nullopt,
		[this](Vignette& vignette, float, float elapsedTime)
		{
			const float duration =
				std::max(config_.vignetteFadeDuration, 0.0001f);
			const float fadeIn = std::clamp(elapsedTime / duration, 0.0f, 1.0f);
			vignette.SetThreshold(config_.vignetteThreshold);
			vignette.SetSoftness(config_.vignetteSoftness);
			vignette.SetIntensity(config_.vignetteIntensity * fadeIn);
			vignette.SetDarkColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		});
}

void GameOverSequence::StartHSVAdjust()
{
	hsvHandle_ = dependencies_.postEffects.Emit<HSVAdjust>(
		"HSVAdjust", std::nullopt,
		[this](HSVAdjust& hsv, float, float elapsedTime)
		{
			const float duration = std::max(config_.hsvFadeDuration, 0.0001f);
			const float fade = std::clamp(elapsedTime / duration, 0.0f, 1.0f);
			hsv.SetHueShift(config_.hueShift * fade);
			hsv.SetSaturation(std::lerp(1.0f, config_.saturation, fade));
			hsv.SetValue(std::lerp(1.0f, config_.value, fade));
			hsv.SetIntensity(1.0f);
		});
}

void GameOverSequence::CancelPostEffects()
{
	if (vignetteHandle_.IsValid())
	{
		dependencies_.postEffects.Cancel(vignetteHandle_);
		vignetteHandle_ = kInvalidPostEffectHandle;
	}
	if (hsvHandle_.IsValid())
	{
		dependencies_.postEffects.Cancel(hsvHandle_);
		hsvHandle_ = kInvalidPostEffectHandle;
	}
}

bool GameOverSequence::HasReachedAngle(float current, float target) const
{
	return target >= 0.0f ? current >= target : current <= target;
}
