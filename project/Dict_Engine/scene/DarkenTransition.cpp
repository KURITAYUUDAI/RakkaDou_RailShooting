#include "DarkenTransition.h"

#include <algorithm>

void DarkenTransition::Begin(TransitionPhase phase)
{
	if (!controller_)
	{
		return;
	}

	if (handle_ != kInvalidPostEffectHandle && !controller_->IsActive(handle_))
	{
		handle_ = kInvalidPostEffectHandle;
	}

	if (handle_ == kInvalidPostEffectHandle)
	{
		handle_ = controller_->Emit<Darken>("Darken", std::nullopt, nullptr);
	}

	const float initialIntensity = phase == TransitionPhase::FadeOut
		? 0.0f
		: std::clamp(setting_.maxIntensity, 0.0f, 1.0f);
	controller_->Edit<Darken>(
		handle_,
		[this, initialIntensity](Darken& darken)
		{
			darken.SetDarkColor(setting_.fadeColor);
			darken.SetIntensity(initialIntensity);
		});
}

void DarkenTransition::Apply(TransitionPhase phase, float progress)
{
	if (!controller_)
	{
		return;
	}

	progress = std::clamp(progress, 0.0f, 1.0f);
	const float maxIntensity = std::clamp(setting_.maxIntensity, 0.0f, 1.0f);
	const float intensity = phase == TransitionPhase::FadeOut
		? maxIntensity * progress
		: maxIntensity * (1.0f - progress);

	controller_->Edit<Darken>(
		handle_,
		[this, intensity](Darken& darken)
		{
			darken.SetDarkColor(setting_.fadeColor);
			darken.SetIntensity(intensity);
		});
}

void DarkenTransition::End(TransitionPhase phase)
{
	if (!controller_)
	{
		return;
	}

	if (phase == TransitionPhase::FadeOut)
	{
		controller_->Edit<Darken>(
			handle_,
			[this](Darken& darken)
			{
				darken.SetDarkColor(setting_.fadeColor);
				darken.SetIntensity(
					std::clamp(setting_.maxIntensity, 0.0f, 1.0f));
			});
		return;
	}

	controller_->Edit<Darken>(
		handle_,
		[](Darken& darken)
		{
			darken.SetIntensity(0.0f);
		});
	controller_->Cancel(handle_);
	handle_ = kInvalidPostEffectHandle;
}

void DarkenTransition::Cancel()
{
	if (controller_ && handle_ != kInvalidPostEffectHandle)
	{
		controller_->Cancel(handle_);
		handle_ = kInvalidPostEffectHandle;
	}
}
