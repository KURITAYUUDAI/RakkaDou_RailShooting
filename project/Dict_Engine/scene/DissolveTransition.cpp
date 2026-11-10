#include "DissolveTransition.h"

void DissolveTransition::Begin(TransitionPhase phase)
{
    //if (handle_ == kInvalidPostEffectHandle)
    //{
    //    handle_ = controller_->Emit<Dissolve>(
    //        "Dissolve",
    //        std::nullopt,
    //        nullptr);
    //}

    //// handle_ が無効（外部 Clear 等で消えた）なら常に再生成
    //if (!controller_->IsActive(handle_))
    //{
    //    handle_ = controller_->Emit<Dissolve>(
    //        "Dissolve", 
    //        std::nullopt, 
    //        nullptr);
    //}

    if (handle_ != kInvalidPostEffectHandle &&
      !controller_->IsActive(handle_))
    {
        handle_ = kInvalidPostEffectHandle;
    }

    if (handle_ == kInvalidPostEffectHandle)
    {
        handle_ = controller_->Emit<Dissolve>(
			BuiltinCanvas::Final,
            "Dissolve",
            std::nullopt,
            nullptr);
    }

    controller_->Edit<Dissolve>(
        handle_,
        [this, phase](Dissolve& dissolve)
        {
            //dissolve.SetEdgeWidth(setting_.edgeWidth);
            //dissolve.SetNoiseScale(setting_.noiseScale);
            //dissolve.SetFadeColor(setting_.fadeColor);
            dissolve.SetEdgeColor(setting_.edgeColor);

            if (phase == TransitionPhase::FadeOut)
            {
                dissolve.SetThreshold(0.0f);
            } 
            else
            {
                dissolve.SetThreshold(1.0f);
            }
        });
}

void DissolveTransition::Apply(TransitionPhase phase, float progress)
{
    float threshold = 0.0f;

    if (phase == TransitionPhase::FadeOut)
    {
        threshold = progress;
    } 
    else
    {
        threshold = 1.0f - progress;
    }

    controller_->Edit<Dissolve>(
        handle_,
        [threshold](Dissolve& dissolve)
        {
            dissolve.SetThreshold(threshold);
        });
}

void DissolveTransition::End(TransitionPhase phase)
{
    if (phase == TransitionPhase::FadeOut)
    {
        // 完全に隠したままSceneを切り替えるので、まだ消さない
        controller_->Edit<Dissolve>(
            handle_,
            [](Dissolve& dissolve)
            {
                dissolve.SetThreshold(1.0f);
            });

        return;
    }

    // FadeIn完了後はPostEffectを消す
    controller_->Edit<Dissolve>(
        handle_,
        [](Dissolve& dissolve)
        {
            dissolve.SetThreshold(0.0f);
        });

    controller_->Cancel(handle_);
    handle_ = kInvalidPostEffectHandle;
}

void DissolveTransition::Cancel()
{
    if (handle_ != kInvalidPostEffectHandle)
    {
        controller_->Cancel(handle_);
        handle_ = kInvalidPostEffectHandle;
    }
}
