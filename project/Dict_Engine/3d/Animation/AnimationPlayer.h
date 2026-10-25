#pragma once

struct Animation;

class AnimationPlayer
{
public:
    void Play(const Animation* animation, bool isLoop = true);

    void Stop();
    void Pause();
    void Resume();

    void Update(float deltaTime);

// 外部入出力

    // セッター
    void SetPlaybackSpeed(float playbackSpeed) { playbackSpeed_ = playbackSpeed; }
    void SetCurrentTime(float time);

	// ゲッター
    const Animation* GetCurrentAnimation() const { return currentAnimation_; };
    const float GetCurrentTime() const { return currentTime_; };
    const float GetPlaybackSpeed() const { return playbackSpeed_; };

    const bool GetIsPlaying() const { return isPlaying_; };
    const bool GetIsPaused() const { return isPaused_; };
    const bool GetIsLoop() const { return isLoop_; };
    const bool GetIsFinished() const { return isFinished_; };

private:
    const Animation* currentAnimation_ = nullptr;

    float currentTime_ = 0.0f;
    float playbackSpeed_ = 1.0f;

    bool isPlaying_ = false;
    bool isPaused_ = false;
    bool isLoop_ = true;
    bool isFinished_ = false;
};

