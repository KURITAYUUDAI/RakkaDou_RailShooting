#pragma once
#include "TransitionTrack.h"
#include "myMath.h"
#include "PostEffectController.h"
#include "Darken.h"

class DarkenTransition : public ITransitionTrack
{
public:
	struct Setting
	{
		float maxIntensity = 1.0f;
		Vector4 fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	explicit DarkenTransition(PostEffectController* controller)
		: controller_(controller)
	{}

	void SetSetting(const Setting& setting) { setting_ = setting; }

	void Begin(TransitionPhase phase) override;
	void Apply(TransitionPhase phase, float progress) override;
	void End(TransitionPhase phase) override;
	void Cancel() override;

private:
	PostEffectController* controller_ = nullptr;
	PostEffectHandle handle_ = kInvalidPostEffectHandle;
	Setting setting_;
};
