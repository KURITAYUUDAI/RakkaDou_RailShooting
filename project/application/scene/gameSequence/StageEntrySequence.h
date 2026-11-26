#pragma once
#include "GamePlaySequence.h"
#include "../GameplayConfig.h"
#include "Object3d.h"
#include "PostEffectController.h"
#include "Sprite.h"

#include <memory>

class Player;
class DefaultCameraController;

struct StageEntryConfig
{
	struct Timing
	{
		float blackHold = 0.25f;
		float approach = 0.90f;
		float doorOpen = 0.65f;
		float diving = 0.55f;
		float whiteOut = 0.20f;
		float whiteFade = 0.45f;
	} timing;

	struct PlayerPath
	{
		Vector3 start{ 0.0f, -1.7f, -7.0f };
		Vector3 door{ 0.0f, 0.0f, 1.5f };
		Vector3 diveEnd{ 0.0f, 0.0f, 10.5f };
		Vector3 rail = GameplayConfig::kPlayerRailLocalPosition;
	} playerPath;

	Vector3 doorPosition{ 0.0f, 3.2f, 8.0f };
};

class StageEntrySequence final : public GameplaySequence
{
public:
	struct Events
	{
		bool completed = false;
	};

	struct StageEntryDependencies
	{
		Player& player;
		DefaultCameraController& defaultCameraController;
		PostEffectController& postEffectController;
	};

	enum class StageEntryState
	{
		WaitingForFadeIn,
		BlackHold,
		Approaching,
		OpeningDoor,
		Diving,
		WhiteOut,
		Complete,
	};

	explicit StageEntrySequence(
		StageEntryDependencies dependencies,
		StageEntryConfig config = {});

	void NotifyFadeInFinished();
	[[nodiscard]] Events ConsumeEvents();
	[[nodiscard]] bool ShouldDrawGameplayWorld() const
	{
		return false;
	}

    void DrawWorld() override;
    void DrawUI() override;

protected:

	void Initialize() override;
    void OnStart() override;
    SequenceResult OnUpdate(float deltaTime) override;

    void OnFinish() override;
    void OnCancel() override;

private:
	Vector3 CalculateGameplayCameraForwardOffset(
		const Vector3& playerLocalPosition) const
	{
		return {
			-playerLocalPosition.x,
			-playerLocalPosition.y,
			-(kGameplayBattleSpaceCenterZ_ + playerLocalPosition.z)
		};
	}

	static float SmoothStep01(float value)
	{
		const float t = std::clamp(value, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	static float LerpFloat(float start, float end, float t)
	{
		return start + (end - start) * t;
	}

	static Vector3 LerpVector3(
		const Vector3& start, const Vector3& end, float t)
	{
		return {
			LerpFloat(start.x, end.x, t),
			LerpFloat(start.y, end.y, t),
			LerpFloat(start.z, end.z, t),
		};
	}

	void UpdateStageEntry(const float& deltaTime);
	void CompleteStageEntry();
	void UpdateStageEntryObjects();
	void DrawStageEntryDarkSpace();
	void StartStageEntryBlur();
	void UpdateStageEntryBlur(float progress, float maxWidth = 0.02f);
	void StopStageEntryBlur();
	[[nodiscard]] bool IsStageEntryComplete() const
	{
		return stageEntryState_ == StageEntryState::Complete;
	}

	static constexpr float kGameplayBattleSpaceCenterZ_ = 22.5f;

	StageEntryConfig config_;

	StageEntryState stageEntryState_ = StageEntryState::WaitingForFadeIn;
	float stageEntryTimer_ = 0.0f;

	Player& player_;
	DefaultCameraController& defaultCameraController_;
	PostEffectController& postEffectController_;
	float stageEntryDoorProgress_ = 0.0f;

	std::unique_ptr<Object3d> stageEntryBackdrop_ = nullptr;
	std::unique_ptr<Object3d> stageEntryLight_ = nullptr;
	std::unique_ptr<Object3d> stageEntryLeftDoor_ = nullptr;
	std::unique_ptr<Object3d> stageEntryRightDoor_ = nullptr;
	std::array<std::unique_ptr<Object3d>, 3> stageEntryDoorFrames_;
	std::unique_ptr<Sprite> stageEntryWhiteSprite_ = nullptr;

	PostEffectHandle stageEntryBlurHandle_ = kInvalidPostEffectHandle;
	Events pendingEvents_;
	
};

