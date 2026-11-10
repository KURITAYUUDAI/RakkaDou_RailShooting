#include "PostEffectController.h"
#include "Logger.h"

void PostEffectController::Update(float deltaTime)
{
    for (auto& active : activeEffects_)
    {
        if (active.cancelRequested || !active.effect)
        {
            continue;
        }

        active.elapsedTime += deltaTime;

        float progress = 0.0f;

        if (active.duration.has_value())
        {
            if (active.duration.value() <= 0.0f)
            {
                progress = 1.0f;
            } 
            else
            {
                progress = active.elapsedTime / active.duration.value();
                progress = std::clamp(progress, 0.0f, 1.0f);
            }
        }

        if (active.updateFunc)
        {
            active.updateFunc(*active.effect, progress, active.elapsedTime);
        }

        if (active.duration.has_value() && progress >= 1.0f)
        {
            active.cancelRequested = true;
        }
    }

    FlushCancelRequests();
}

void PostEffectController::Cancel(PostEffectHandle handle)
{
    ActiveEffect* active = Find(handle);

    if (!active)
    {
        return;
    }

    active->cancelRequested = true;
}

void PostEffectController::CancelAll()
{
    for (auto& active : activeEffects_)
    {
        active.cancelRequested = true;
    }
}

bool PostEffectController::IsActive(PostEffectHandle handle) const
{
    const ActiveEffect* active = Find(handle);

    if (!active)
    {
        return false;
    }

    if (active->cancelRequested || active->effect == nullptr)
    {
        return false;
    }

    if (!active->chain || !active->chain->HasEffect(active->effect))
    {
        return false;
    }

    return true;
}

bool PostEffectController::SetPriority(PostEffectHandle handle, int32_t priority)
{
    ActiveEffect* activeEffect = Find(handle);

    if (!activeEffect)
    {
        return false;
    }

    if (!activeEffect->chain)
    {
        return false;
    }

    return activeEffect->chain->SetPriority(activeEffect->effect, priority);
}

PostEffectController::ActiveEffect* PostEffectController::Find(PostEffectHandle handle)
{
    if (!handle.IsValid())
    {
        return nullptr;
    }

    for (auto& active : activeEffects_)
    {
        if (active.handle == handle)
        {
            return &active;
        }
    }

    return nullptr;
}

const PostEffectController::ActiveEffect* PostEffectController::Find(PostEffectHandle handle) const
{
    if (!handle.IsValid())
    {
        return nullptr;
    }

    for (const auto& active : activeEffects_)
    {
        if (active.handle == handle)
        {
            return &active;
        }
    }

    return nullptr;
}

void PostEffectController::FlushCancelRequests()
{
    for (auto& active : activeEffects_)
    {
        if (active.cancelRequested && active.effect)
        {
        #ifdef _DEBUG
            Logger::Log("FlushCancelRequests: removing effect ptr=" + std::to_string(reinterpret_cast<uintptr_t>(active.effect)) +
                " name=" + active.effect->GetName() + "\n");
        #endif
            if (active.chain)
            {
                active.chain->Remove(active.effect);
            }
            active.effect = nullptr;
            active.chain = nullptr;
        }
    }

    activeEffects_.erase(
        std::remove_if(
            activeEffects_.begin(),
            activeEffects_.end(),
            [](const ActiveEffect& active){
                return active.effect == nullptr;
            }),
        activeEffects_.end());
}
