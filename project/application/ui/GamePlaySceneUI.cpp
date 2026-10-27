#include "GamePlaySceneUI.h"
#include "HPGageUI.h"
#include "LockOnUI.h"
#include "DirectXBase.h"
#include "CameraManager.h"
#include "time/DeltaTimeManager.h"

void GamePlaySceneUI::Initialize(EventBus* eventBus)
{
    uiManager_ = std::make_unique<UIManager<GameUIViewModel>>();
    uiManager_->Initialize();

    gameUIController_ = std::make_unique<GameUIController>();
    gameUIController_->Initialize(eventBus);

	keyboardControlText_ = std::make_unique<Text>();
	const auto keyboardLabel = [this](InputAction action)
	{
		return inputMapping_->GetBindingLabel(
			InputContext::Gameplay, action, InputDevice::Keyboard);
	};
	keyboardControlText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf",
		30,
		keyboardLabel(InputAction::Move) + U" : MOVE\n" +
		keyboardLabel(InputAction::Shot) + U" : SHOT\n" +
		keyboardLabel(InputAction::Avoid) + U" : AVOID\n" +
		keyboardLabel(InputAction::LockOn) + U" : LOCKON\n" +
		keyboardLabel(InputAction::Melee) + U" : MELEE\n" +
		keyboardLabel(InputAction::Pause) + U" : PAUSE");
	keyboardControlText_->SetPosition({ 10.0f, 500.0f });
	keyboardControlText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	controllerControlText_ = std::make_unique<Text>();
	const auto controllerLabel = [this](InputAction action)
	{
		return inputMapping_->GetBindingLabel(
			InputContext::Gameplay, action, InputDevice::Gamepad);
	};
	controllerControlText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf",
		30,
		controllerLabel(InputAction::Move) + U" : MOVE\n" +
		controllerLabel(InputAction::Shot) + U" : SHOT\n" +
		controllerLabel(InputAction::Avoid) + U" : AVOID\n" +
		controllerLabel(InputAction::LockOn) + U" : LOCKON\n" +
		controllerLabel(InputAction::Melee) + U" : MELEE\n" +
		controllerLabel(InputAction::Pause) + U" : PAUSE");
	controllerControlText_->SetPosition({ 10.0f, 500.0f });
	controllerControlText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    auto playerHPGage = std::make_unique<HPGageUI>();
    HPGageUIConfig playerConfig{};
    playerConfig.backPos = { 32.0f, 32.0f };
    playerConfig.fillPos = { 36.0f, 36.0f };
    playerConfig.backSize = { 308.0f, 40.0f };
    playerConfig.fillSize = { 300.0f, 32.0f };
    playerConfig.backColor = { 0.8f, 0.8f, 0.8f, 1.0f };
    playerConfig.fillColor = { 0.0f, 1.0f, 1.0f, 1.0f };
    playerConfig.direction = HPGageDirection::LeftToRight;
    playerConfig.layer = UILayer::Screen;

    playerHPGage->SetConfig(playerConfig);

    playerHPGage->SetHPGetter(
        [](const GameUIViewModel& viewModel) -> const HPViewModel&
        {
            return viewModel.playerHitPoint;
        }
    );
    playerHPGage->Initialize();

    uiManager_->AddUI(std::move(playerHPGage));

   /* auto lockOn = std::make_unique<LockOnUI>();*/
    

    /*auto enemyHPGage = std::make_unique<HPGageUI>();
    HPGageUIConfig enemyConfig{};
    enemyConfig.backPos = { 1248.0f, 32.0f };
    enemyConfig.fillPos = { 1244.0f, 36.0f };
    enemyConfig.backSize = { 308.0f, 40.0f };
    enemyConfig.fillSize = { 300.0f, 32.0f };
    enemyConfig.backColor = { 0.8f, 0.8f, 0.8f, 1.0f };
    enemyConfig.fillColor = { 1.0f, 0.0f, 0.0f, 1.0f };
    enemyConfig.direction = HPGageDirection::RightToLeft;
    enemyConfig.layer = UILayer::Screen;

    enemyHPGage->SetConfig(enemyConfig);

    enemyHPGage->SetHPGetter(
        [](const GameUIViewModel& viewModel) -> const HPViewModel&{
            return viewModel.bossHitPoint;
        }
    );

    enemyHPGage->Initialize();

    uiManager_->AddUI(std::move(enemyHPGage));*/
}

void GamePlaySceneUI::Finalize()
{
	if (keyboardControlText_)
    {
		keyboardControlText_->Finalize();
		keyboardControlText_.reset();
	}

	if (controllerControlText_)
	{
		controllerControlText_->Finalize();
		controllerControlText_.reset();
    }

    if (gameUIController_)
    {
        gameUIController_->Finalize();
        gameUIController_.reset();
    }

    uiManager_.reset();
}

void GamePlaySceneUI::Update(float deltaTime)
{
    deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::UI);

    gameUIController_->Update(deltaTime_);

    SyncScreenBossHPGage();
    SyncOverHeadEnemyHPGages();
    SyncLockOnUI();

    uiManager_->Update(gameUIController_->GetViewModel(), deltaTime_);
	keyboardControlText_->Update();
	controllerControlText_->Update();
}

void GamePlaySceneUI::DrawLayer(const UILayer& layer)
{
    uiManager_->DrawLayer(layer);

	if (isVisible_ && layer == UILayer::Screen)
	{
		if (inputMapping_->GetActiveDevice() == InputDevice::Gamepad)
		{
			controllerControlText_->Draw();
		} else
		{
			keyboardControlText_->Draw();
		}
	}
}

void GamePlaySceneUI::SyncScreenBossHPGage()
{
    const EnemyHPViewModel* target = FindScreenBossTarget();

    if (!target)
    {
        if (screenBossHPGageUIID_ != kInvalidUIID)
        {
            uiManager_->RemoveUI(screenBossHPGageUIID_);
            screenBossHPGageUIID_ = kInvalidUIID;
            screenBossEnemyID_ = 0;
        }

        return;
    }

    if (screenBossHPGageUIID_ != kInvalidUIID &&
        screenBossEnemyID_ == target->enemyID)
    {
        return;
    }

    if (screenBossHPGageUIID_ != kInvalidUIID)
    {
        uiManager_->RemoveUI(screenBossHPGageUIID_);
        screenBossHPGageUIID_ = kInvalidUIID;
    }

    screenBossEnemyID_ = target->enemyID;

    auto hpGage = std::make_unique<HPGageUI>();

    HPGageUIConfig config{};
    config.backPos = { 1248.0f, 32.0f };
    config.fillPos = { 1244.0f, 36.0f };
    config.backSize = { 308.0f, 40.0f };
    config.fillSize = { 300.0f, 32.0f };
    config.backColor = { 0.8f, 0.8f, 0.8f, 1.0f };
    config.fillColor = { 1.0f, 0.0f, 0.0f, 1.0f };
    config.direction = HPGageDirection::RightToLeft;
    config.layer = UILayer::Screen;

    hpGage->SetConfig(config);

    EnemyID enemyID = target->enemyID;

    hpGage->SetHPGetter(
        [enemyID](const GameUIViewModel& viewModel) -> const HPViewModel&{
            for (const auto& enemyHP : viewModel.enemyHitPoints)
            {
                if (enemyHP.enemyID == enemyID)
                {
                    return enemyHP.hitPoint;
                }
            }

            static HPViewModel dummy{};
            return dummy;
        }
    );

    hpGage->Initialize();

    screenBossHPGageUIID_ = uiManager_->AddUI(std::move(hpGage));
}

void GamePlaySceneUI::SyncOverHeadEnemyHPGages()
{
    const GameUIViewModel& viewModel = gameUIController_->GetViewModel();

    for (const auto& enemyHP : viewModel.enemyHitPoints)
    {
        EnemyID enemyID = enemyHP.enemyID;

		if (!viewModel.isLockOn || viewModel.targetEnemyID != enemyID)
		{
			continue;
		}

        if (!enemyHP.isVisible)
        {
            continue;
        }

        if (!HasEnemyHPGageDisplay(
            enemyHP.displayType,
            EnemyHPGageDisplayType::OverHead))
        {
            continue;
        }

        if (overHeadHPGageUIIDs_.contains(enemyID))
        {
            continue;
        }

        auto hpGage = std::make_unique<HPGageUI>();

        HPGageUIConfig config{};
        config.backPos = enemyHP.screenPosition;
        config.fillPos = enemyHP.screenPosition;
        config.backSize = { 100.0f, 8.0f };
        config.fillSize = { 100.0f, 8.0f };
        config.backColor = { 0.2f, 0.2f, 0.2f, 1.0f };
        config.fillColor = { 0.8f, 0.0f, 0.0f, 1.0f };
        config.direction = HPGageDirection::RightToLeft;
        config.layer = UILayer::Screen;

        hpGage->SetConfig(config);

        hpGage->SetHPGetter(
            [enemyID](const GameUIViewModel& viewModel) -> const HPViewModel&{
                for (const auto& enemyHP : viewModel.enemyHitPoints)
                {
                    if (enemyHP.enemyID == enemyID)
                    {
                        return enemyHP.hitPoint;
                    }
                }

                static HPViewModel dummy{};
                return dummy;
            }
        );

        hpGage->SetPositionGetter(
            [enemyID](const GameUIViewModel& viewModel) -> Vector2{
                for (const auto& enemyHP : viewModel.enemyHitPoints)
                {
                    if (enemyHP.enemyID == enemyID)
                    {
                        return enemyHP.screenPosition;
                    }
                }

                return {};
            }
        );

        hpGage->Initialize();

        UIID uiID = uiManager_->AddUI(std::move(hpGage));
        overHeadHPGageUIIDs_[enemyID] = uiID;
    }

    for (auto it = overHeadHPGageUIIDs_.begin();
         it != overHeadHPGageUIIDs_.end(); )
    {
        EnemyID enemyID = it->first;

        bool shouldExist = false;

        for (const auto& enemyHP : viewModel.enemyHitPoints)
        {
            if (enemyHP.enemyID != enemyID)
            {
                continue;
            }

            shouldExist =
				viewModel.isLockOn &&
				viewModel.targetEnemyID == enemyID &&
                enemyHP.isVisible &&
                HasEnemyHPGageDisplay(
                    enemyHP.displayType,
                    EnemyHPGageDisplayType::OverHead);

            break;
        }

        if (!shouldExist)
        {
            uiManager_->RemoveUI(it->second);
            it = overHeadHPGageUIIDs_.erase(it);
        } else
        {
            ++it;
        }
    }
}

void GamePlaySceneUI::SyncLockOnUI()
{
    const GameUIViewModel& viewModel = gameUIController_->GetViewModel();

    // 1. ロックオンしていない場合
    if (!viewModel.isLockOn)
    {
        // すでにUIが存在するなら削除する
        if (lockOnUIID_ != kInvalidUIID)
        {
            uiManager_->RemoveUI(lockOnUIID_);
            lockOnUIID_ = kInvalidUIID; // 無効値に戻す
        }
        return;
    }

    // 2. ロックオン中、かつすでにUIが生成されている場合は何もしない
    if (lockOnUIID_ != kInvalidUIID)
    {
        return;
    }

    // 3. ロックオン中、かつUIがまだ未生成の場合のみ作成して Add する

    auto lockOnUI = std::make_unique<LockOnUI>();

    // 座標取得用ラムダの登録
    lockOnUI->SetPositionGetter(
        [](const GameUIViewModel& viewModel) -> Vector2
        {
            Matrix4x4 vpMatrix = CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix();
            Matrix4x4 viewport = DirectXBase::GetInstance()->GetViewportMatrix();
            Matrix4x4 screenMatrix = vpMatrix * viewport;

			const Vector3& worldPosition = viewModel.targetEnemyWorldPosition;
			const float screenX = worldPosition.x * screenMatrix.m[0][0] +
				worldPosition.y * screenMatrix.m[1][0] +
				worldPosition.z * screenMatrix.m[2][0] + screenMatrix.m[3][0];
			const float screenY = worldPosition.x * screenMatrix.m[0][1] +
				worldPosition.y * screenMatrix.m[1][1] +
				worldPosition.z * screenMatrix.m[2][1] + screenMatrix.m[3][1];
			const float w = worldPosition.x * screenMatrix.m[0][3] +
				worldPosition.y * screenMatrix.m[1][3] +
				worldPosition.z * screenMatrix.m[2][3] + screenMatrix.m[3][3];
			if (w <= 0.000001f)
			{
				return { -10000.0f, -10000.0f };
			}
			return { screenX / w, screenY / w };
        }
    );

    // 表示フラグ取得用ラムダの登録
    lockOnUI->SetVisibilityGetter(
        [](const GameUIViewModel& viewModel) -> bool
        {
           return viewModel.isLockOn;
        }
    );

    lockOnUI->Initialize();

    lockOnUIID_ = uiManager_->AddUI(std::move(lockOnUI));
}

const EnemyHPViewModel* GamePlaySceneUI::FindScreenBossTarget() const
{
    const GameUIViewModel& viewModel = gameUIController_->GetViewModel();

    const EnemyHPViewModel* result = nullptr;

    for (const auto& enemyHP : viewModel.enemyHitPoints)
    {
        if (!enemyHP.isVisible)
        {
            continue;
        }

        if (!HasEnemyHPGageDisplay(
            enemyHP.displayType,
            EnemyHPGageDisplayType::ScreenBoss))
        {
            continue;
        }

        if (!result ||
            enemyHP.screenBossPriority > result->screenBossPriority)
        {
            result = &enemyHP;
        }
    }

    return result;
}


