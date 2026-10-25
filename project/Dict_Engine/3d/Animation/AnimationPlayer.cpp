#include "AnimationPlayer.h"
#include "AnimationUtility.h"

void AnimationPlayer::Play(const Animation* animation, bool isLoop)
{
    currentAnimation_ = animation;
    currentTime_ = 0.0f;

    isLoop_ = isLoop;
    isPaused_ = false;
    isFinished_ = false;
    isPlaying_ = currentAnimation_ != nullptr;
}

void AnimationPlayer::Update(float deltaTime)
{
    if (!isPlaying_ || isPaused_ || currentAnimation_ == nullptr) 
    {
        return;
    }

    const float duration = currentAnimation_->duration;

    if (duration <= 0.0f) 
    {
        currentTime_ = 0.0f;
        isPlaying_ = false;
        isFinished_ = true;
        return;
    }

    currentTime_ += deltaTime * playbackSpeed_;

    if (isLoop_) 
    {
        currentTime_ = std::fmod(currentTime_, duration);

        if (currentTime_ < 0.0f) 
        {
            currentTime_ += duration;
        }
    } 
    else if (currentTime_ >= duration) 
    {
        currentTime_ = duration;
        isPlaying_ = false;
        isFinished_ = true;
    } 
    else if (currentTime_ < 0.0f) 
    {
        currentTime_ = 0.0f;
        isPlaying_ = false;
        isFinished_ = true;
    }
}

void AnimationPlayer::Stop()
{
    currentTime_ = 0.0f;
    isPlaying_ = false;
    isPaused_ = false;
    isFinished_ = false;
}

void AnimationPlayer::Pause()
{
    if (isPlaying_) 
    {
        isPaused_ = true;
    }
}

void AnimationPlayer::Resume()
{
    if (currentAnimation_ != nullptr && !isFinished_) 
    {
        isPlaying_ = true;
        isPaused_ = false;
    }
}

void AnimationPlayer::SetCurrentTime(float time)
{
    if (currentAnimation_ == nullptr || currentAnimation_->duration <= 0.0f) 
    {
        currentTime_ = 0.0f;
        return;
    }

    const float duration = currentAnimation_->duration;

    if (isLoop_) 
    {
        currentTime_ = std::fmod(time, duration);

        if (currentTime_ < 0.0f) 
        {
            currentTime_ += duration;
        }
    } 
    else 
    {
        currentTime_ = std::clamp(time, 0.0f, duration);
    }
}