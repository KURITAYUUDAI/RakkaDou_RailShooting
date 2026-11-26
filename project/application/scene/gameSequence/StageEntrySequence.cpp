#include "StageEntrySequence.h"
#include "ModelManager.h"
#include "PrimitiveManager.h"
#include "WindowsAPI.h"

#include "Player.h"
#include "DefaultCameraController.h"
#include "PostEffectController.h"
#include "RadialBlur.h"

#include <utility>

void StageEntrySequence::OnStart()
{
	if (!stageEntryBackdrop_)
	{
		Initialize();
	}

	stageEntryState_ = StageEntryState::WaitingForFadeIn;
	stageEntryTimer_ = 0.0f;
	stageEntryDoorProgress_ = 0.0f;
	pendingEvents_ = {};

	player_.SetParent(nullptr);
	player_.SetScale({ 1.0f, 1.0f, 1.0f });
	player_.SetRotate({ 0.0f, 0.0f, 0.0f });
	player_.SetTranslate(config_.playerPath.start);
	player_.SetTitleOpacity(1.0f);

	defaultCameraController_.GetWorldTransform()->SetTranslate(
		config_.playerPath.start +
		CalculateGameplayCameraForwardOffset(config_.playerPath.rail));

	stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	stageEntryWhiteSprite_->Update();
	UpdateStageEntryObjects();
}

SequenceResult StageEntrySequence::OnUpdate(float deltaTime)
{
	UpdateStageEntry(deltaTime);
	return IsStageEntryComplete()
		? SequenceResult::Completed
		: SequenceResult::Running;
}

void StageEntrySequence::OnFinish()
{
	StopStageEntryBlur();
}

void StageEntrySequence::OnCancel()
{
	StopStageEntryBlur();
	if (stageEntryWhiteSprite_)
	{
		stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		stageEntryWhiteSprite_->Update();
	}
}

StageEntrySequence::StageEntrySequence(
	StageEntryDependencies dependencies,
	StageEntryConfig config)
	: player_(dependencies.player),
	defaultCameraController_(dependencies.defaultCameraController),
	postEffectController_(dependencies.postEffectController),
	config_(std::move(config))
{
}

void StageEntrySequence::NotifyFadeInFinished()
{
	if (!GetIsActive() ||
		stageEntryState_ != StageEntryState::WaitingForFadeIn)
	{
		return;
	}

	stageEntryTimer_ = 0.0f;
	stageEntryState_ = StageEntryState::BlackHold;
}

StageEntrySequence::Events StageEntrySequence::ConsumeEvents()
{
	const Events events = pendingEvents_;
	pendingEvents_ = {};
	return events;
}

void StageEntrySequence::DrawWorld()
{
	if (!GetIsActive())
	{
		return;
	}

	DrawStageEntryDarkSpace();
}

void StageEntrySequence::DrawUI()
{
	if (!GetIsActive() || !stageEntryWhiteSprite_)
	{
		return;
	}

	if (stageEntryWhiteSprite_->GetColor().w > 0.0f)
	{
		stageEntryWhiteSprite_->Draw();
	}
}


void StageEntrySequence::Initialize()
{
	if (stageEntryBackdrop_)
	{
		return;
	}

	PrimitiveManager::GetInstance()->CreateBox("stage_entry_box");

	const auto createBox = [this](const Vector3& scale, const Vector3& translate,
		const Vector4& color){
			auto object = std::make_unique<Object3d>();
			object->Initialize();
			object->SetModel("stage_entry_box");
			object->SetEnableLighting(false);
			object->SetScale(scale);
			object->SetTranslate(translate);
			object->SetColor(color);
			return object;
		};

	stageEntryBackdrop_ = createBox(
		{ 40.0f, 24.0f, 0.5f }, { 0.0f, 0.0f, 10.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f });
	stageEntryLight_ = createBox(
		{ 8.0f, 10.0f, 0.15f }, { 0.0f, 0.0f, 8.4f },
		{ 8.0f, 8.0f, 8.0f, 1.0f });
	stageEntryLeftDoor_ = createBox(
		{ 4.0f, 10.0f, 0.35f }, { -2.0f, 0.0f, 8.0f },
		{ 0.015f, 0.02f, 0.035f, 1.0f });
	stageEntryRightDoor_ = createBox(
		{ 4.0f, 10.0f, 0.35f }, { 2.0f, 0.0f, 8.0f },
		{ 0.015f, 0.02f, 0.035f, 1.0f });

	stageEntryDoorFrames_[0] = createBox(
		{ 0.6f, 11.0f, 0.6f }, { -4.3f, 0.0f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });
	stageEntryDoorFrames_[1] = createBox(
		{ 0.6f, 11.0f, 0.6f }, { 4.3f, 0.0f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });
	stageEntryDoorFrames_[2] = createBox(
		{ 9.2f, 0.6f, 0.6f }, { 0.0f, 5.3f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });

	stageEntryWhiteSprite_ = std::make_unique<Sprite>();
	stageEntryWhiteSprite_->Initialize("white1x1.png");
	stageEntryWhiteSprite_->SetSize({
		static_cast<float>(WindowsAPI::kClientWidth),
		static_cast<float>(WindowsAPI::kClientHeight) });
	stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	stageEntryWhiteSprite_->Update();
}

void StageEntrySequence::UpdateStageEntry(const float& deltaTime)
{
#ifdef _DEBUG
	ImGui::Begin("StageEntry");
	ImGui::Text("State: %d", static_cast<int>(stageEntryState_));
	ImGui::Text("Timer: %.3f", stageEntryTimer_);
	const char* stageEntryStateNames[] = {
		"WaitingForFadeIn", "BlackHold", "Approaching", "OpeningDoor",
		"Diving", "WhiteOut", "Complete"
	};
	const int stageEntryStateIndex = static_cast<int>(stageEntryState_);
	ImGui::Text("Sequence: %s", stageEntryStateNames[stageEntryStateIndex]);
	Vector3 currentPlayerLocalPosition = player_.GetTranslate();
	Vector3 currentPlayerWorldPosition = player_.GetWorldPosition();
	ImGui::InputFloat3(
		"CurrentPlayerLocal", &currentPlayerLocalPosition.x,
		"%.3f", ImGuiInputTextFlags_ReadOnly);
	ImGui::InputFloat3(
		"CurrentPlayerWorld", &currentPlayerWorldPosition.x,
		"%.3f", ImGuiInputTextFlags_ReadOnly);
	ImGui::SeparatorText("Player Sequence Positions");
	ImGui::DragFloat3(
		"BlackHold / Approach Start", &config_.playerPath.start.x, 0.1f);
	ImGui::DragFloat3(
		"Approach End / Door", &config_.playerPath.door.x, 0.1f);
	ImGui::DragFloat3(
		"Diving End / WhiteOut", &config_.playerPath.diveEnd.x, 0.1f);
	ImGui::DragFloat3(
		"Door Position", &config_.doorPosition.x, 0.1f);
	ImGui::End();
#endif

	switch (stageEntryState_)
	{
	case StageEntryState::WaitingForFadeIn:
		break;

	case StageEntryState::BlackHold:
		stageEntryTimer_ += deltaTime;
		if (stageEntryTimer_ >= config_.timing.blackHold)
		{
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::Approaching;
		}
		break;

	case StageEntryState::Approaching:
	{
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / config_.timing.approach, 0.0f, 1.0f);
		const float easedProgress = SmoothStep01(progress);
		const Vector3 playerPosition = LerpVector3(
			config_.playerPath.start, config_.playerPath.door, easedProgress);
		player_.SetTranslate(playerPosition);
		defaultCameraController_.GetWorldTransform()->SetTranslate(
			playerPosition +
			CalculateGameplayCameraForwardOffset(config_.playerPath.rail));

		if (progress >= 1.0f)
		{
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::OpeningDoor;
		}
		break;
	}

	case StageEntryState::OpeningDoor:
	{
		player_.SetTranslate(config_.playerPath.door);
		defaultCameraController_.GetWorldTransform()->SetTranslate(
			config_.playerPath.door +
			CalculateGameplayCameraForwardOffset(config_.playerPath.rail));
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / config_.timing.doorOpen, 0.0f, 1.0f);
		stageEntryDoorProgress_ = SmoothStep01(progress);
		if (progress >= 1.0f)
		{
			StartStageEntryBlur();
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::Diving;
		}
		break;
	}

	case StageEntryState::Diving:
	{
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / config_.timing.diving, 0.0f, 1.0f);
		const float easedProgress = progress * progress;
		const Vector3 playerPosition = LerpVector3(
			config_.playerPath.door, config_.playerPath.diveEnd, easedProgress);
		player_.SetTranslate(playerPosition);
		defaultCameraController_.GetWorldTransform()->SetTranslate(
			playerPosition +
			CalculateGameplayCameraForwardOffset(config_.playerPath.rail));
		UpdateStageEntryBlur(easedProgress);

		if (progress >= 1.0f)
		{
			StopStageEntryBlur();
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::WhiteOut;
		}
		break;
	}

	case StageEntryState::WhiteOut:
	{
		player_.SetTranslate(config_.playerPath.diveEnd);
		stageEntryTimer_ += deltaTime;
		float alpha = 0.0f;
		if (stageEntryTimer_ < config_.timing.whiteOut)
		{
			const float fadeInProgress = std::clamp(
				stageEntryTimer_ / config_.timing.whiteOut, 0.0f, 1.0f);
			alpha = SmoothStep01(fadeInProgress);
		}
		else
		{
			const float fadeOutProgress = std::clamp(
				(stageEntryTimer_ - config_.timing.whiteOut) /
				config_.timing.whiteFade,
				0.0f, 1.0f);
			alpha = 1.0f - SmoothStep01(fadeOutProgress);
			if (fadeOutProgress >= 1.0f)
			{
				CompleteStageEntry();
			}
		}
		stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		break;
	}

	case StageEntryState::Complete:
		return;
	}

	UpdateStageEntryObjects();
	if (stageEntryWhiteSprite_)
	{
		stageEntryWhiteSprite_->Update();
	}
}

void StageEntrySequence::CompleteStageEntry()
{
	stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	stageEntryTimer_ = 0.0f;
	stageEntryState_ = StageEntryState::Complete;
	pendingEvents_.completed = true;
}

void StageEntrySequence::UpdateStageEntryObjects()
{
	constexpr float doorHalfWidth = 2.0f;
	const float leftHingeX = config_.doorPosition.x - 4.0f;
	const float rightHingeX = config_.doorPosition.x + 4.0f;
	const float leftAngle = -stageEntryDoorProgress_ * (pi / 2.0f);
	const float rightAngle = stageEntryDoorProgress_ * (pi / 2.0f);

	stageEntryBackdrop_->SetTranslate({
		config_.doorPosition.x,
		config_.doorPosition.y,
		config_.doorPosition.z + 2.0f });
	stageEntryLight_->SetTranslate({
		config_.doorPosition.x,
		config_.doorPosition.y,
		config_.doorPosition.z + 0.4f });
	stageEntryLeftDoor_->SetRotate({ 0.0f, leftAngle, 0.0f });
	stageEntryLeftDoor_->SetTranslate({
		leftHingeX + std::cos(leftAngle) * doorHalfWidth,
		config_.doorPosition.y,
		config_.doorPosition.z - std::sin(leftAngle) * doorHalfWidth });
	stageEntryRightDoor_->SetRotate({ 0.0f, rightAngle, 0.0f });
	stageEntryRightDoor_->SetTranslate({
		rightHingeX - std::cos(rightAngle) * doorHalfWidth,
		config_.doorPosition.y,
		config_.doorPosition.z + std::sin(rightAngle) * doorHalfWidth });
	stageEntryDoorFrames_[0]->SetTranslate({
		config_.doorPosition.x - 4.3f,
		config_.doorPosition.y,
		config_.doorPosition.z - 0.1f });
	stageEntryDoorFrames_[1]->SetTranslate({
		config_.doorPosition.x + 4.3f,
		config_.doorPosition.y,
		config_.doorPosition.z - 0.1f });
	stageEntryDoorFrames_[2]->SetTranslate({
		config_.doorPosition.x,
		config_.doorPosition.y + 5.3f,
		config_.doorPosition.z - 0.1f });

	stageEntryBackdrop_->Update();
	stageEntryLight_->Update();
	stageEntryLeftDoor_->Update();
	stageEntryRightDoor_->Update();
	for (auto& frame : stageEntryDoorFrames_)
	{
		frame->Update();
	}
}

void StageEntrySequence::StartStageEntryBlur()
{
	if (postEffectController_.IsActive(stageEntryBlurHandle_))
	{
		return;
	}

	stageEntryBlurHandle_ = postEffectController_.Emit<RadialBlur>(
		"RadialBlur", std::nullopt, nullptr);
	postEffectController_.Edit<RadialBlur>(
		stageEntryBlurHandle_,
		[](RadialBlur& blur){
			blur.SetCenter({ 0.5f, 0.5f });
			blur.SetBlurWidth(0.0f);
		});
}

void StageEntrySequence::UpdateStageEntryBlur(float progress, float maxWidth)
{
	if (!postEffectController_.IsActive(stageEntryBlurHandle_))
	{
		return;
	}

	const float blurWidth =
		(std::max)(0.0f, maxWidth) * std::clamp(progress, 0.0f, 1.0f);
	postEffectController_.Edit<RadialBlur>(
		stageEntryBlurHandle_,
		[blurWidth](RadialBlur& blur){
			blur.SetBlurWidth(blurWidth);
		});
}

void StageEntrySequence::StopStageEntryBlur()
{
	if (stageEntryBlurHandle_ == kInvalidPostEffectHandle)
	{
		return;
	}

	postEffectController_.Cancel(stageEntryBlurHandle_);
	stageEntryBlurHandle_ = kInvalidPostEffectHandle;
}

void StageEntrySequence::DrawStageEntryDarkSpace()
{
	stageEntryBackdrop_->Draw();
	stageEntryLight_->Draw();
	stageEntryLeftDoor_->Draw();
	stageEntryRightDoor_->Draw();
	for (auto& frame : stageEntryDoorFrames_)
	{
		frame->Draw();
	}
}

