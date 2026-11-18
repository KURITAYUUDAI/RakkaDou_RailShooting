#pragma once
#include "UIControllerBase.h"
#include "GamePlayUIUtility.h"

class GameUIController : public UIControllerBase
{
public:
   
    void Update(float deltaTime) override;
    void SyncSnapshot(const GameUIFrameSnapshot& snapshot);

    const GameUIViewModel& GetViewModel() const { return viewModel_; }

private:

    GameUIViewModel viewModel_;
    bool hasSnapshot_ = false;
};
