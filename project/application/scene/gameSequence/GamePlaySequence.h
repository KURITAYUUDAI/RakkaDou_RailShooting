#pragma once
#include "myMath.h"

enum class SequenceResult
{
    Running,
    Completed,
};

class GameplaySequence
{
public:
    
    virtual ~GameplaySequence() = default;

    void Start();

    SequenceResult Update(float deltaTime);

    void Cancel();

    bool GetIsActive() const { return isActive_; }

    virtual void DrawWorld() {}
    virtual void DrawUI() {}

protected:
    
    virtual void Initialize() = 0;

    virtual void OnStart() = 0;
    virtual SequenceResult OnUpdate(float deltaTime) = 0;

    virtual void OnFinish() = 0;
    virtual void OnCancel() = 0;

private:
    
    bool isActive_ = false;
};