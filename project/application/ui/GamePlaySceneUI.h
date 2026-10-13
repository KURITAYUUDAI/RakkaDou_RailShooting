#pragma once
#include <memory>
#include <unordered_map>
#include "UIManager.h"
#include "SceneUIBase.h"
#include "GameUIController.h"
#include "GamePlayUIUtility.h"
#include "Text.h"

class GamePlaySceneUI : public SceneUIBase
{
public:
    void Initialize(EventBus* eventBus) override;
    void Finalize() override;
    void Update(float deltaTime) override;
    void DrawLayer(const UILayer& layer) override;

    void SetIsVisible(bool visible) override
    {
        isVisible_ = visible;
        uiManager_->SetIsVisible(visible);
    }

private:
    void SyncScreenBossHPGage();
    void SyncOverHeadEnemyHPGages();
    void SyncLockOnUI();

    const EnemyHPViewModel* FindScreenBossTarget() const;

private:

    float deltaTime_ = 0.0f;

    std::unique_ptr<UIManager<GameUIViewModel>> uiManager_;
    std::unique_ptr<GameUIController> gameUIController_;
    std::unique_ptr<Text> controlText_;
    bool isVisible_ = true;

    UIID screenBossHPGageUIID_ = kInvalidUIID;
    EnemyID screenBossEnemyID_ = 0;

    std::unordered_map<EnemyID, UIID> overHeadHPGageUIIDs_;
    UIID lockOnUIID_ = kInvalidUIID;
};

