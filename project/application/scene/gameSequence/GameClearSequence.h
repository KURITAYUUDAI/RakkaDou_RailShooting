#pragma once

#include "GamePlaySequence.h"
#include "PostEffectController.h"
#include "myMath.h"

#include <memory>

class ParticleEmitter;
class Player;
class Text;

struct GameClearDependencies
{
	Player& player;
	PostEffectController& postEffects;
};

struct GameClearConfig
{
	Vector3 playerTargetPosition = { 0.0f, -1.7f, -7.5f };
	float repositionDuration = 1.0f;
	float textDuration = 2.0f;
	float diveDuration = 1.0f;
	float diveDistance = 500.0f;
	float transitionDelay = 0.5f;
	float blurMaxWidth = 0.015f;
};

class GameClearSequence final : public GamePlaySequence
{
public:
	GameClearSequence(
		GameClearDependencies dependencies,
		GameClearConfig config = {});
	~GameClearSequence() override;

	void Start() override;
	SequenceStatus Update(float deltaTime) override;
	SequenceCompletionAction GetCompletionAction() const override;
	SequencePolicy GetPolicy() const override;

	void DrawWorld() override;
	void DrawOverlay() override;
	void DrawUI() override;

private:
	enum class State
	{
		RepositioningPlayer,
		DisplayingText,
		Diving,
		WaitingForTransition,
		Complete,
	};

	void InitializeAssets();
	void StartDive();
	void StartBlur();
	void UpdateBlur(float progress);
	void StopBlur();

private:
	GameClearDependencies dependencies_;
	GameClearConfig config_;

	State state_ = State::RepositioningPlayer;
	float stateTimer_ = 0.0f;
	Vector3 playerStartPosition_{};
	Vector3 diveStartPosition_{};
	std::unique_ptr<Text> clearText_;
	std::unique_ptr<ParticleEmitter> shockwaveEmitter_;
	PostEffectHandle blurHandle_ = kInvalidPostEffectHandle;
};
