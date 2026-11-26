#define NOMINMAX
#include "GameClearSequence.h"

#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Player.h"
#include "PrimitiveManager.h"
#include "RadialBlur.h"
#include "Text.h"

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

GameClearSequence::GameClearSequence(
	GameClearDependencies dependencies,
	GameClearConfig config)
	: dependencies_(dependencies),
	  config_(config)
{
}

GameClearSequence::~GameClearSequence()
{
	StopBlur();
	if (clearText_)
	{
		clearText_->Finalize();
	}
}

void GameClearSequence::Start()
{
	InitializeAssets();
	state_ = State::RepositioningPlayer;
	stateTimer_ = 0.0f;
	playerStartPosition_ = dependencies_.player.GetTranslate();

	clearText_ = std::make_unique<Text>();
	clearText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf", 64, U"STAGE CLEAR");
	clearText_->SetAnchorPoint({ 0.5f, 0.5f });
	clearText_->SetPosition({ 640.0f, 240.0f });
	clearText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	clearText_->Update();

	dependencies_.player.StartStageClearSequence();
}

void GameClearSequence::InitializeAssets()
{
	PrimitiveManager::RingConfig shockwaveConfig;
	shockwaveConfig.segments = 48;
	shockwaveConfig.doubleSided = true;
	shockwaveConfig.innerRadius = 1.5f;
	shockwaveConfig.outerRadius = 2.0f;
	shockwaveConfig.innerColor = { 0.0f, 0.4f, 2.0f, 0.8f };
	shockwaveConfig.outerColor = { 0.0f, 2.0f, 5.0f, 1.0f };
	shockwaveConfig.uvScaleU = 2.0f;
	shockwaveConfig.uvScaleV = 0.1f;
	shockwaveConfig.startAngle = 0.0f;
	shockwaveConfig.endAngle = 2.0f * pi;
	PrimitiveManager::GetInstance()->CreateRing(
		"stage_clear_shockwave", shockwaveConfig);

	auto* particleManager = ParticleManager::GetInstance();
	particleManager->CreateParticleGroup(
		"stage_clear_shockwave", "gradationLine.png");
	particleManager->SetModel(
		"stage_clear_shockwave", "stage_clear_shockwave");
	particleManager->SetIsMoveAccelerationField(
		"stage_clear_shockwave", false);
	particleManager->SetIsBillboard("stage_clear_shockwave", true);
	particleManager->SetScaleOverLife(
		"stage_clear_shockwave",
		{ 0.75f, 0.75f, 0.75f }, { 8.0f, 8.0f, 8.0f });
	particleManager->SetParticleLifeTime("stage_clear_shockwave", 0.5f);

	shockwaveEmitter_ = std::make_unique<ParticleEmitter>();
	shockwaveEmitter_->Initialize(
		"stage_clear_shockwave",
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} },
		1, 0.2f);
}

SequenceStatus GameClearSequence::Update(float deltaTime)
{
	switch (state_)
	{
	case State::RepositioningPlayer:
	{
		stateTimer_ += deltaTime;
		const float duration = std::max(config_.repositionDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		dependencies_.player.SetTranslate(LerpVector3(
			playerStartPosition_, config_.playerTargetPosition,
			SmoothStep01(progress)));
		if (progress >= 1.0f)
		{
			dependencies_.player.SetTranslate(config_.playerTargetPosition);
			stateTimer_ = 0.0f;
			state_ = State::DisplayingText;
		}
		break;
	}

	case State::DisplayingText:
		stateTimer_ += deltaTime;
		clearText_->Update();
		if (stateTimer_ >= config_.textDuration)
		{
			StartDive();
		}
		break;

	case State::Diving:
	{
		stateTimer_ += deltaTime;
		clearText_->Update();
		const float duration = std::max(config_.diveDuration, 0.001f);
		const float progress = std::clamp(stateTimer_ / duration, 0.0f, 1.0f);
		const float easedProgress = progress * progress;
		Vector3 playerPosition = diveStartPosition_;
		playerPosition.z += config_.diveDistance * easedProgress;
		dependencies_.player.SetTranslate(playerPosition);
		UpdateBlur(easedProgress);
		if (progress >= 1.0f)
		{
			stateTimer_ = 0.0f;
			state_ = State::WaitingForTransition;
		}
		break;
	}

	case State::WaitingForTransition:
		stateTimer_ += deltaTime;
		if (stateTimer_ >= config_.transitionDelay)
		{
			state_ = State::Complete;
			return SequenceStatus::Finished;
		}
		break;

	case State::Complete:
		return SequenceStatus::Finished;
	}

	return SequenceStatus::Running;
}

SequenceCompletionAction GameClearSequence::GetCompletionAction() const
{
	return SequenceCompletionAction::ReturnToTitle;
}

SequencePolicy GameClearSequence::GetPolicy() const
{
	SequencePolicy policy{};
	policy.world = WorldPresentation::GameplayWorld;
	policy.simulation = GameplaySimulation::Frozen;
	policy.stageProgression = StageProgression::Running;
	policy.playerControl = PlayerControl::Sequence;
	policy.playerVisibility = PlayerVisibility::Visible;
	policy.gameplayActors = GameplayActorVisibility::Visible;
	policy.hud = HudVisibility::Visible;
	policy.pauseEnabled = false;
	return policy;
}

void GameClearSequence::DrawWorld()
{
}

void GameClearSequence::DrawOverlay()
{
}

void GameClearSequence::DrawUI()
{
	if ((state_ == State::DisplayingText || state_ == State::Diving) &&
		clearText_)
	{
		clearText_->Draw();
	}
}

void GameClearSequence::StartDive()
{
	diveStartPosition_ = dependencies_.player.GetTranslate();
	shockwaveEmitter_->SetTranslate(
		dependencies_.player.GetWorldPosition());
	shockwaveEmitter_->Emit();
	StartBlur();
	stateTimer_ = 0.0f;
	state_ = State::Diving;
}

void GameClearSequence::StartBlur()
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

void GameClearSequence::UpdateBlur(float progress)
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

void GameClearSequence::StopBlur()
{
	if (!blurHandle_.IsValid())
	{
		return;
	}

	dependencies_.postEffects.Cancel(blurHandle_);
	blurHandle_ = kInvalidPostEffectHandle;
}
