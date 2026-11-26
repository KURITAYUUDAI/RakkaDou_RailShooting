#define NOMINMAX
#include "StageEntrySequence.h"

#include "Camera.h"
#include "CameraManager.h"
#include "DefaultCameraController.h"
#include "Object3d.h"
#include "Player.h"
#include "RadialBlur.h"
#include "RailCameraController.h"
#include "Sprite.h"
#include "WindowsAPI.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kGameplayBattleSpaceCenterZ = 22.5f;

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

	Vector3 CalculateGameplayCameraForwardOffset(
		const Vector3& playerLocalPosition)
	{
		return {
			-playerLocalPosition.x,
			-playerLocalPosition.y,
			-(kGameplayBattleSpaceCenterZ + playerLocalPosition.z),
		};
	}
}

StageEntrySequence::StageEntrySequence(
	StageEntryDependencies dependencies,
	StageEntryConfig config)
	: dependencies_(dependencies),
	  config_(config)
{
}

StageEntrySequence::~StageEntrySequence()
{
	StopBlur();
}

void StageEntrySequence::Start()
{
	InitializeAssets();

	state_ = State::WaitingForFadeIn;
	stateTimer_ = 0.0f;
	doorProgress_ = 0.0f;
	lookDownCameraTurnTimer_ = 0.0f;

	dependencies_.cameraManager.SetActiveCameraController("Default");
	dependencies_.defaultCamera.GetWorldTransform()->SetTranslate(
		config_.playerStartPosition +
		CalculateGameplayCameraForwardOffset(
			config_.gameplayPlayerRailPosition));
	dependencies_.defaultCamera.GetWorldTransform()->SetRotate(
		{ 0.0f, 0.0f, 0.0f });

	dependencies_.player.SetParent(nullptr);
	dependencies_.player.SetScale({ 1.0f, 1.0f, 1.0f });
	dependencies_.player.SetRotate({ 0.0f, 0.0f, 0.0f });
	dependencies_.player.SetTranslate(config_.playerStartPosition);
	dependencies_.player.SetTitleOpacity(0.0f);

	whiteOverlay_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	StopBlur();
	UpdateEntryObjects();
}

SequenceStatus StageEntrySequence::Update(float deltaTime)
{
	if (state_ == State::Complete)
	{
		return SequenceStatus::Finished;
	}

	switch (state_)
	{
	case State::WaitingForFadeIn:
		break;

	case State::BlackHold:
		stateTimer_ += deltaTime;
		if (stateTimer_ >= config_.blackHoldDuration)
		{
			stateTimer_ = 0.0f;
			state_ = State::Approaching;
		}
		break;

	case State::Approaching:
	{
		stateTimer_ += deltaTime;
		const float duration = std::max(config_.approachDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		const float easedProgress = SmoothStep01(progress);
		const Vector3 playerPosition = LerpVector3(
			config_.playerStartPosition,
			config_.playerDoorPosition,
			easedProgress);
		dependencies_.player.SetTranslate(playerPosition);
		dependencies_.player.SetTitleOpacity(
			std::clamp(progress / 0.35f, 0.0f, 1.0f));
		dependencies_.defaultCamera.GetWorldTransform()->SetTranslate(
			playerPosition +
			CalculateGameplayCameraForwardOffset(
				config_.gameplayPlayerRailPosition));

		if (progress >= 1.0f)
		{
			stateTimer_ = 0.0f;
			state_ = State::OpeningDoor;
		}
		break;
	}

	case State::OpeningDoor:
	{
		dependencies_.player.SetTranslate(config_.playerDoorPosition);
		dependencies_.defaultCamera.GetWorldTransform()->SetTranslate(
			config_.playerDoorPosition +
			CalculateGameplayCameraForwardOffset(
				config_.gameplayPlayerRailPosition));
		stateTimer_ += deltaTime;
		const float duration = std::max(config_.doorOpenDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		doorProgress_ = SmoothStep01(progress);
		if (progress >= 1.0f)
		{
			StartBlur();
			stateTimer_ = 0.0f;
			state_ = State::Diving;
		}
		break;
	}

	case State::Diving:
	{
		stateTimer_ += deltaTime;
		const float duration = std::max(config_.diveDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		const float easedProgress = progress * progress;
		const Vector3 playerPosition = LerpVector3(
			config_.playerDoorPosition,
			config_.playerDiveEndPosition,
			easedProgress);
		dependencies_.player.SetTranslate(playerPosition);
		dependencies_.defaultCamera.GetWorldTransform()->SetTranslate(
			playerPosition +
			CalculateGameplayCameraForwardOffset(
				config_.gameplayPlayerRailPosition));
		UpdateBlur(easedProgress);

		if (progress >= 1.0f)
		{
			StopBlur();
			stateTimer_ = 0.0f;
			state_ = State::WhiteOut;
		}
		break;
	}

	case State::WhiteOut:
	{
		dependencies_.player.SetTranslate(config_.playerDiveEndPosition);
		stateTimer_ += deltaTime;
		const float duration = std::max(config_.whiteOutDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		whiteOverlay_->SetColor(
			{ 1.0f, 1.0f, 1.0f, SmoothStep01(progress) });
		if (progress >= 1.0f)
		{
			BeginLookDown();
		}
		break;
	}

	case State::LookDown:
	{
		stateTimer_ += deltaTime;
		lookDownCameraTurnTimer_ += deltaTime;
		const float duration = std::max(config_.whiteFadeDuration, 0.001f);
		const float whiteProgress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		whiteOverlay_->SetColor(
			{ 1.0f, 1.0f, 1.0f, 1.0f - SmoothStep01(whiteProgress) });
		break;
	}

	case State::Complete:
		break;
	}

	dependencies_.cameraManager.Update(deltaTime);
	if (state_ == State::LookDown)
	{
		dependencies_.railCamera.SetBattleSpaceParent(
			dependencies_.railCamera.GetWorldTransform());
		UpdateLookDownCamera();

		const float lookDownDuration =
			config_.lookDownForwardHoldDuration +
			config_.lookDownTurnDuration;
		if (lookDownCameraTurnTimer_ >= lookDownDuration)
		{
			state_ = State::Complete;
		}
	}

	UpdateEntryObjects();
	whiteOverlay_->Update();

	return state_ == State::Complete
		? SequenceStatus::Finished
		: SequenceStatus::Running;
}

SequenceCompletionAction StageEntrySequence::GetCompletionAction() const
{
	return SequenceCompletionAction::BeginGameplay;
}

SequencePolicy StageEntrySequence::GetPolicy() const
{
	SequencePolicy policy{};
	policy.world = state_ == State::LookDown || state_ == State::Complete
		? WorldPresentation::GameplayWorld
		: WorldPresentation::EntryRoom;
	policy.simulation = GameplaySimulation::Frozen;
	policy.stageProgression = StageProgression::Frozen;
	policy.playerControl = PlayerControl::Sequence;
	policy.playerVisibility =
		state_ == State::WaitingForFadeIn || state_ == State::BlackHold
		? PlayerVisibility::Hidden
		: PlayerVisibility::Visible;
	policy.gameplayActors = GameplayActorVisibility::Hidden;
	policy.hud = HudVisibility::Hidden;
	policy.pauseEnabled = false;
	return policy;
}

void StageEntrySequence::DrawWorld()
{
	if (state_ == State::LookDown || state_ == State::Complete)
	{
		return;
	}

	backdrop_->Draw();
	if (state_ == State::WaitingForFadeIn || state_ == State::BlackHold)
	{
		return;
	}

	light_->Draw();
	leftDoor_->Draw();
	rightDoor_->Draw();
	for (auto& frame : doorFrames_)
	{
		frame->Draw();
	}
}

void StageEntrySequence::DrawOverlay()
{
	if (whiteOverlay_ && whiteOverlay_->GetColor().w > 0.0f)
	{
		whiteOverlay_->Draw();
	}
}

void StageEntrySequence::DrawUI()
{
}

void StageEntrySequence::OnFadeInFinished()
{
	if (state_ != State::WaitingForFadeIn)
	{
		return;
	}

	stateTimer_ = 0.0f;
	state_ = State::BlackHold;
}

void StageEntrySequence::InitializeAssets()
{
	if (assetsInitialized_)
	{
		return;
	}

	const auto createBox = [](const Vector3& scale, const Vector3& translate,
		const Vector4& color)
		{
			auto object = std::make_unique<Object3d>();
			object->Initialize();
			object->SetModel("stage_entry_box");
			object->SetEnableLighting(false);
			object->SetScale(scale);
			object->SetTranslate(translate);
			object->SetColor(color);
			return object;
		};

	backdrop_ = createBox(
		{ 40.0f, 24.0f, 0.5f }, { 0.0f, 0.0f, 10.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f });
	light_ = createBox(
		{ 8.0f, 10.0f, 0.15f }, { 0.0f, 0.0f, 8.4f },
		{ 8.0f, 8.0f, 8.0f, 1.0f });
	leftDoor_ = createBox(
		{ 4.0f, 10.0f, 0.35f }, { -2.0f, 0.0f, 8.0f },
		{ 0.015f, 0.02f, 0.035f, 1.0f });
	rightDoor_ = createBox(
		{ 4.0f, 10.0f, 0.35f }, { 2.0f, 0.0f, 8.0f },
		{ 0.015f, 0.02f, 0.035f, 1.0f });

	doorFrames_[0] = createBox(
		{ 0.6f, 11.0f, 0.6f }, { -4.3f, 0.0f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });
	doorFrames_[1] = createBox(
		{ 0.6f, 11.0f, 0.6f }, { 4.3f, 0.0f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });
	doorFrames_[2] = createBox(
		{ 9.2f, 0.6f, 0.6f }, { 0.0f, 5.3f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });

	whiteOverlay_ = std::make_unique<Sprite>();
	whiteOverlay_->Initialize("white1x1.png");
	whiteOverlay_->SetSize({
		static_cast<float>(WindowsAPI::kClientWidth),
		static_cast<float>(WindowsAPI::kClientHeight),
	});
	whiteOverlay_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	assetsInitialized_ = true;
}

void StageEntrySequence::BeginLookDown()
{
	StopBlur();

	stateTimer_ = 0.0f;
	lookDownCameraTurnTimer_ = 0.0f;
	state_ = State::LookDown;

	const float initialSpeed = std::clamp(
		config_.skydivingInitialSpeed,
		0.0f,
		config_.normalRailSpeed);
	dependencies_.railCamera.SetMovementEnabled(false);
	dependencies_.railCamera.SetVerticalStartHeight(
		config_.skydivingStartHeight);
	dependencies_.railCamera.SetMoveSpeed(initialSpeed);
	dependencies_.cameraManager.SetActiveCameraController("Rail");
	dependencies_.railCamera.SetBattleSpaceParent(
		dependencies_.railCamera.GetWorldTransform());

	dependencies_.player.SetTranslate(config_.gameplayPlayerRailPosition);
	dependencies_.player.SetRotate({ 0.0f, 0.0f, 0.0f });
	dependencies_.player.SetParent(
		dependencies_.railCamera.GetBattleWorldTransform());

	const float heightCorrection =
		config_.skydivingStartHeight -
		dependencies_.player.GetWorldPosition().y;
	dependencies_.railCamera.SetVerticalStartHeight(
		config_.skydivingStartHeight + heightCorrection);
	dependencies_.railCamera.SetBattleSpaceParent(
		dependencies_.railCamera.GetWorldTransform());
	dependencies_.player.SetParent(
		dependencies_.railCamera.GetBattleWorldTransform());

	UpdateLookDownCamera();
}

void StageEntrySequence::UpdateLookDownCamera()
{
	const Vector3 gameplayAnchor = TransformPosition(
		config_.gameplayPlayerRailPosition,
		dependencies_.railCamera.GetBattleWorldTransform()->GetWorldMatrix());
	const float turnDuration = std::max(config_.lookDownTurnDuration, 0.001f);
	const float turnProgress = std::clamp(
		(lookDownCameraTurnTimer_ - config_.lookDownForwardHoldDuration) /
		turnDuration,
		0.0f,
		1.0f);
	const float easedProgress = SmoothStep01(turnProgress);
	const Quaternion railRotation =
		dependencies_.railCamera.GetWorldTransform()->GetRotateQuat();
	const Quaternion cameraRotation = Slerp(
		IdentityQuaternion(), railRotation, easedProgress);
	const Vector3 railCameraOffset =
		dependencies_.railCamera.GetWorldPosition() - gameplayAnchor;
	const Vector3 localCameraOffset = RotateVector(
		railCameraOffset, Inverse(railRotation));
	const Vector3 cameraOffset = RotateVector(
		localCameraOffset, cameraRotation);
	const Vector3 cameraPosition = gameplayAnchor + cameraOffset;

	Camera* mainCamera = dependencies_.cameraManager.GetMainCamera();
	mainCamera->SetTranslate(cameraPosition);
	mainCamera->SetRotateQuat(cameraRotation);
	mainCamera->Update();
	dependencies_.cameraManager.SetCameraWorldPosition(cameraPosition);
}

void StageEntrySequence::UpdateEntryObjects()
{
	constexpr float doorHalfWidth = 2.0f;
	const float leftHingeX = config_.doorPosition.x - 4.0f;
	const float rightHingeX = config_.doorPosition.x + 4.0f;
	const float leftAngle = -doorProgress_ * (pi / 2.0f);
	const float rightAngle = doorProgress_ * (pi / 2.0f);

	backdrop_->SetTranslate({
		config_.doorPosition.x,
		config_.doorPosition.y,
		config_.doorPosition.z + 2.0f,
	});
	light_->SetTranslate({
		config_.doorPosition.x,
		config_.doorPosition.y,
		config_.doorPosition.z + 0.4f,
	});
	leftDoor_->SetRotate({ 0.0f, leftAngle, 0.0f });
	leftDoor_->SetTranslate({
		leftHingeX + std::cos(leftAngle) * doorHalfWidth,
		config_.doorPosition.y,
		config_.doorPosition.z - std::sin(leftAngle) * doorHalfWidth,
	});
	rightDoor_->SetRotate({ 0.0f, rightAngle, 0.0f });
	rightDoor_->SetTranslate({
		rightHingeX - std::cos(rightAngle) * doorHalfWidth,
		config_.doorPosition.y,
		config_.doorPosition.z + std::sin(rightAngle) * doorHalfWidth,
	});
	doorFrames_[0]->SetTranslate({
		config_.doorPosition.x - 4.3f,
		config_.doorPosition.y,
		config_.doorPosition.z - 0.1f,
	});
	doorFrames_[1]->SetTranslate({
		config_.doorPosition.x + 4.3f,
		config_.doorPosition.y,
		config_.doorPosition.z - 0.1f,
	});
	doorFrames_[2]->SetTranslate({
		config_.doorPosition.x,
		config_.doorPosition.y + 5.3f,
		config_.doorPosition.z - 0.1f,
	});

	backdrop_->Update();
	light_->Update();
	leftDoor_->Update();
	rightDoor_->Update();
	for (auto& frame : doorFrames_)
	{
		frame->Update();
	}
}

void StageEntrySequence::StartBlur()
{
	if (dependencies_.postEffects.IsActive(blurHandle_))
	{
		return;
	}

	blurHandle_ = dependencies_.postEffects.Emit<RadialBlur>(
		"RadialBlur", std::nullopt, nullptr);
	dependencies_.postEffects.Edit<RadialBlur>(
		blurHandle_,
		[](RadialBlur& blur)
		{
			blur.SetCenter({ 0.5f, 0.5f });
			blur.SetBlurWidth(0.0f);
		});
}

void StageEntrySequence::UpdateBlur(float progress)
{
	if (!dependencies_.postEffects.IsActive(blurHandle_))
	{
		return;
	}

	const float blurWidth = std::max(0.0f, config_.blurMaxWidth) *
		std::clamp(progress, 0.0f, 1.0f);
	dependencies_.postEffects.Edit<RadialBlur>(
		blurHandle_,
		[blurWidth](RadialBlur& blur)
		{
			blur.SetBlurWidth(blurWidth);
		});
}

void StageEntrySequence::StopBlur()
{
	if (blurHandle_ == kInvalidPostEffectHandle)
	{
		return;
	}

	dependencies_.postEffects.Cancel(blurHandle_);
	blurHandle_ = kInvalidPostEffectHandle;
}
