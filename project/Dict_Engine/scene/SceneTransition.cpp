#include "SceneTransition.h"
#include <algorithm>

class ISceneTransitionState
{
public:
    virtual ~ISceneTransitionState() = default;

    virtual SceneTransition::SceneTransitionState GetType() const = 0;
    virtual void Enter(SceneTransition&) {}
    virtual std::unique_ptr<ISceneTransitionState> Update(
        SceneTransition& transition, float deltaTime) = 0;
};

class SceneTransitionIdleState final : public ISceneTransitionState
{
public:
    SceneTransition::SceneTransitionState GetType() const override
    {
        return SceneTransition::SceneTransitionState::Idle;
    }

    void Enter(SceneTransition& transition) override
    {
        transition.timer_ = 0.0f;
    }

    std::unique_ptr<ISceneTransitionState> Update(
        SceneTransition&, float) override;
};

class SceneTransitionFadeOutState final : public ISceneTransitionState
{
public:
    SceneTransition::SceneTransitionState GetType() const override
    {
        return SceneTransition::SceneTransitionState::FadeOut;
    }

    void Enter(SceneTransition& transition) override;
    std::unique_ptr<ISceneTransitionState> Update(
        SceneTransition& transition, float deltaTime) override;
};

class SceneTransitionFadeInState final : public ISceneTransitionState
{
public:
    SceneTransition::SceneTransitionState GetType() const override
    {
        return SceneTransition::SceneTransitionState::FadeIn;
    }

    void Enter(SceneTransition& transition) override;
    std::unique_ptr<ISceneTransitionState> Update(
        SceneTransition& transition, float deltaTime) override;
};

std::unique_ptr<ISceneTransitionState> SceneTransitionIdleState::Update(
    SceneTransition&, float)
{
    return nullptr;
}

void SceneTransitionFadeOutState::Enter(SceneTransition& transition)
{
    transition.timer_ = 0.0f;
    if (transition.track_)
    {
        transition.track_->Begin(TransitionPhase::FadeOut);
        transition.track_->Apply(TransitionPhase::FadeOut, 0.0f);
    }
}

std::unique_ptr<ISceneTransitionState> SceneTransitionFadeOutState::Update(
    SceneTransition& transition, float deltaTime)
{
    transition.timer_ += deltaTime;
    if (!transition.track_)
    {
        transition.Finish();
        return std::make_unique<SceneTransitionIdleState>();
    }

    const float progress = transition.fadeOutDuration_ > 0.0f
        ? std::clamp(
            transition.timer_ / transition.fadeOutDuration_, 0.0f, 1.0f)
        : 1.0f;
    transition.track_->Apply(TransitionPhase::FadeOut, progress);
    if (progress < 1.0f)
    {
        return nullptr;
    }

    transition.track_->End(TransitionPhase::FadeOut);
    return std::make_unique<SceneTransitionFadeInState>();
}

void SceneTransitionFadeInState::Enter(SceneTransition& transition)
{
    transition.timer_ = 0.0f;
    if (transition.onSwitchScene_)
    {
        transition.onSwitchScene_();
    }
    if (transition.track_)
    {
        transition.track_->Begin(TransitionPhase::FadeIn);
        transition.track_->Apply(TransitionPhase::FadeIn, 0.0f);
    }
}

std::unique_ptr<ISceneTransitionState> SceneTransitionFadeInState::Update(
    SceneTransition& transition, float deltaTime)
{
    transition.timer_ += deltaTime;
    if (!transition.track_)
    {
        transition.Finish();
        return std::make_unique<SceneTransitionIdleState>();
    }

    const float progress = transition.fadeInDuration_ > 0.0f
        ? std::clamp(
            transition.timer_ / transition.fadeInDuration_, 0.0f, 1.0f)
        : 1.0f;
    transition.track_->Apply(TransitionPhase::FadeIn, progress);
    if (progress < 1.0f)
    {
        return nullptr;
    }

    transition.track_->End(TransitionPhase::FadeIn);
    transition.Finish();
    return std::make_unique<SceneTransitionIdleState>();
}

SceneTransition::SceneTransition()
    : currentState_(std::make_unique<SceneTransitionIdleState>())
{
}

SceneTransition::~SceneTransition() = default;

void SceneTransition::Start(
	float fadeOutDuration, 
	float fadeInDuration, 
	std::function<void()> onSwitchScene, 
	std::unique_ptr<ITransitionTrack> track)
{
    if (GetState() != SceneTransitionState::Idle)
    {
        return;
    }

	fadeOutDuration_ = fadeOutDuration;
	fadeInDuration_ = fadeInDuration;

    onSwitchScene_ = std::move(onSwitchScene);
    track_ = std::move(track);
    ChangeState(std::make_unique<SceneTransitionFadeOutState>());
}

void SceneTransition::Update(float deltaTime)
{
    if (!currentState_)
    {
        return;
    }

    std::unique_ptr<ISceneTransitionState> nextState =
        currentState_->Update(*this, deltaTime);
    if (nextState)
    {
        ChangeState(std::move(nextState));
    }
}

SceneTransition::SceneTransitionState SceneTransition::GetState() const
{
    return currentState_
        ? currentState_->GetType()
        : SceneTransitionState::Idle;
}

bool SceneTransition::IsRunning() const
{
    return GetState() != SceneTransitionState::Idle;
}

bool SceneTransition::IsSceneChangeTiming() const
{
	return false;
}

void SceneTransition::ChangeState(
    std::unique_ptr<ISceneTransitionState> nextState)
{
	currentState_ = std::move(nextState);
	if (currentState_)
	{
		currentState_->Enter(*this);
	}
}

void SceneTransition::Finish()
{
    if (onFinish_)
    {
        onFinish_();
    }

    timer_ = 0.0f;
    onSwitchScene_ = nullptr;
	onFinish_ = nullptr;
    track_.reset();
}
