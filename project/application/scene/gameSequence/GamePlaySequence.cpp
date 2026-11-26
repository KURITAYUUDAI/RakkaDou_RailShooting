#include "GamePlaySequence.h"

void GameplaySequence::Start()
{
    if (isActive_)
    {
        return;
    }

    isActive_ = true;
    OnStart();
}

SequenceResult GameplaySequence::Update(float deltaTime)
{
    if (!isActive_)
    {
        return SequenceResult::Completed;
    }

    const SequenceResult result = OnUpdate(deltaTime);

    if (result == SequenceResult::Completed)
    {
        isActive_ = false;
        OnFinish();
    }

    return result;
}

void GameplaySequence::Cancel()
{
    if (!isActive_)
    {
        return;
    }

    isActive_ = false;
    OnCancel();
}
