#pragma once
#include <functional>
#include <memory>
#include "TransitionTrack.h"

class ISceneTransitionState;
class SceneTransitionIdleState;
class SceneTransitionFadeOutState;
class SceneTransitionFadeInState;

class SceneTransition
{
public:
    enum class SceneTransitionState
    {
        Idle,
        FadeOut,
        FadeIn
    };

    
public:
    SceneTransition();
    ~SceneTransition();

    void Start(
        float fadeOutDuration,
        float fadeInDuration,
        std::function<void()> onSwitchScene,
        std::unique_ptr<ITransitionTrack> track);

    void Update(float deltaTime);

public:
    SceneTransitionState GetState() const;
    bool IsRunning() const;
    bool IsSceneChangeTiming() const;

	void SetOnFinish(std::function<void()> onFinish) { onFinish_ = std::move(onFinish); }

private:
    friend class SceneTransitionIdleState;
    friend class SceneTransitionFadeOutState;
    friend class SceneTransitionFadeInState;

    void ChangeState(std::unique_ptr<ISceneTransitionState> nextState);
    void Finish();

private:
    std::unique_ptr<ISceneTransitionState> currentState_;

    float timer_= 0.0f;
    float fadeOutDuration_ = 0.5f;
    float fadeInDuration_ = 0.5f;

    std::function<void()> onSwitchScene_;

    std::unique_ptr<ITransitionTrack> track_;

	std::function<void()> onFinish_;
};

