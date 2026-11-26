#define NOMINMAX
#include "TitleEntranceSequence.h"

#include "Camera.h"
#include "DefaultCameraController.h"
#include "Player.h"
#include "Sprite.h"

#include <algorithm>

namespace
{
	float SmoothStep01(float value)
	{
		const float t = std::clamp(value, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	Vector3 LerpVector3(const Vector3& start, const Vector3& end, float t)
	{
		return {
			start.x + (end.x - start.x) * t,
			start.y + (end.y - start.y) * t,
			start.z + (end.z - start.z) * t,
		};
	}
}

TitleEntranceSequence::TitleEntranceSequence(
	TitleEntranceDependencies dependencies,
	TitleEntranceConfig config)
	: dependencies_(dependencies),
	  config_(config)
{
}

void TitleEntranceSequence::Start()
{
	state_ = State::PlayerFalling;
	stateTimer_ = 0.0f;
	dependencies_.player.SetTranslate(config_.playerStartPosition);
	dependencies_.player.SetTitleOpacity(0.0f);
	SetCameraTransform(config_.cameraStartPosition, config_.cameraSideRotation);
	dependencies_.titleSprite.SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	dependencies_.openingFadeSprite.SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
}

TitleSequenceStatus TitleEntranceSequence::Update(float deltaTime)
{
	switch (state_)
	{
	case State::PlayerFalling:
	{
		stateTimer_ += deltaTime;
		const float duration = std::max(config_.fallDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		dependencies_.player.SetTranslate(LerpVector3(
			config_.playerStartPosition,
			config_.playerFinalPosition,
			progress * progress));
		dependencies_.player.SetTitleOpacity(SmoothStep01(progress));
		SetCameraTransform(
			LerpVector3(
				config_.cameraStartPosition,
				config_.cameraSidePosition,
				progress),
			config_.cameraSideRotation);

		const float fadeDuration =
			std::max(config_.openingFadeDuration, 0.001f);
		const float fadeProgress =
			std::clamp(stateTimer_ / fadeDuration, 0.0f, 1.0f);
		dependencies_.openingFadeSprite.SetColor(
			{ 0.0f, 0.0f, 0.0f, 1.0f - fadeProgress });

		if (progress >= 1.0f)
		{
			dependencies_.player.SetTranslate(config_.playerFinalPosition);
			dependencies_.player.SetTitleOpacity(1.0f);
			SetCameraTransform(
				config_.cameraSidePosition,
				config_.cameraSideRotation);
			dependencies_.openingFadeSprite.SetColor(
				{ 0.0f, 0.0f, 0.0f, 0.0f });
			stateTimer_ = 0.0f;
			state_ = State::WaitingForCamera;
		}
		break;
	}

	case State::WaitingForCamera:
		stateTimer_ += deltaTime;
		if (stateTimer_ >= std::max(config_.cameraDelay, 0.0f))
		{
			stateTimer_ = 0.0f;
			state_ = State::MovingCamera;
		}
		break;

	case State::MovingCamera:
	{
		stateTimer_ += deltaTime;
		const float duration =
			std::max(config_.cameraMoveDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		const float cameraProgress = SmoothStep01(progress);
		SetCameraTransform(
			LerpVector3(
				config_.cameraSidePosition,
				config_.cameraFinalPosition,
				cameraProgress),
			LerpVector3(
				config_.cameraSideRotation,
				config_.cameraFinalRotation,
				cameraProgress));
		if (progress >= 1.0f)
		{
			SetCameraTransform(
				config_.cameraFinalPosition,
				config_.cameraFinalRotation);
			stateTimer_ = 0.0f;
			state_ = State::FadingTitle;
		}
		break;
	}

	case State::FadingTitle:
	{
		stateTimer_ += deltaTime;
		const float duration =
			std::max(config_.titleFadeDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		dependencies_.titleSprite.SetColor(
			{ 1.0f, 1.0f, 1.0f, SmoothStep01(progress) });
		if (progress >= 1.0f)
		{
			Complete();
		}
		break;
	}

	case State::Complete:
		break;
	}

	return state_ == State::Complete
		? TitleSequenceStatus::Finished
		: TitleSequenceStatus::Running;
}

bool TitleEntranceSequence::HandleConfirm()
{
	if (state_ == State::Complete)
	{
		return false;
	}

	Complete();
	return true;
}

TitleSequenceCompletionAction
TitleEntranceSequence::GetCompletionAction() const
{
	return TitleSequenceCompletionAction::BeginExit;
}

bool TitleEntranceSequence::IsComplete() const
{
	return state_ == State::Complete;
}

void TitleEntranceSequence::DrawOverlay(bool drawTitle)
{
	if (drawTitle &&
		(state_ == State::FadingTitle || state_ == State::Complete))
	{
		dependencies_.titleSprite.Draw();
	}
	if (dependencies_.openingFadeSprite.GetColor().w > 0.0f)
	{
		dependencies_.openingFadeSprite.Draw();
	}
}

void TitleEntranceSequence::SetCameraTransform(
	const Vector3& translate,
	const Vector3& rotate)
{
	dependencies_.cameraController.GetWorldTransform()->SetTranslate(translate);
	dependencies_.cameraController.GetWorldTransform()->SetRotate(rotate);
	dependencies_.camera.SetTranslate(translate);
	dependencies_.camera.SetRotate(rotate);
}

void TitleEntranceSequence::Complete()
{
	dependencies_.player.SetTranslate(config_.playerFinalPosition);
	dependencies_.player.SetTitleOpacity(1.0f);
	SetCameraTransform(config_.cameraFinalPosition, config_.cameraFinalRotation);
	dependencies_.openingFadeSprite.SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	dependencies_.titleSprite.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	stateTimer_ = 0.0f;
	state_ = State::Complete;
}
