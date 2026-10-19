#include "GameUIController.h"


void GameUIController::Update(float deltaTime)
{
    deltaTime;
}

void GameUIController::RegisterEvents()
{
    if (!eventSubscriber_.GetEventBus())
    {
        return;
    }

    eventSubscriber_.Subscribe<PlayerHPChangeEvent>(
        [this](const PlayerHPChangeEvent& event)
        {
            OnPlayerHPChanged(event);
        }
    );

    eventSubscriber_.Subscribe<EnemyAppierEvent>(
        [this](const EnemyAppierEvent& event){
            OnEnemyAppier(event);
        }
    );

    eventSubscriber_.Subscribe<EnemyScreenPositionEvent>(
        [this](const EnemyScreenPositionEvent& event){
            OnEnemyScreenPosition(event);
        }
    );

    eventSubscriber_.Subscribe<EnemyHPChangeEvent>(
        [this](const EnemyHPChangeEvent& event){
            OnEnemyHPChanged(event);
        }
    );

    eventSubscriber_.Subscribe<NearestEnemyInfoEvent>(
        [this](const NearestEnemyInfoEvent& event)
        {
            OnNearestEnemyInfo(event);
        }
    );

    eventSubscriber_.Subscribe<PlayerLockOnEvent>(
        [this](const PlayerLockOnEvent& event){
            OnPlayerLockOn(event);
        }
    );
}

void GameUIController::OnPlayerHPChanged(const PlayerHPChangeEvent& event)
{
	viewModel_.playerHitPoint.currentHitPoint = event.currentHitPoint;
    viewModel_.playerHitPoint.previousHitPoint = event.previousHitPoint;
	viewModel_.playerHitPoint.maxHitPoint = event.maxHitPoint;
}

void GameUIController::OnEnemyAppier(const EnemyAppierEvent& event)
{
    EnemyHPViewModel enemyHP{};

    enemyHP.enemyID = event.enemyID;

    enemyHP.hitPoint.currentHitPoint = event.currentHitPoint;
    enemyHP.hitPoint.previousHitPoint = event.previousHitPoint;
    enemyHP.hitPoint.maxHitPoint = event.maxHitPoint;

    enemyHP.screenPosition = event.screenPosition;
    enemyHP.isVisible = event.isVisible;

    enemyHP.displayType = event.displayType;
    enemyHP.screenBossPriority = event.screenBossPriority;

    viewModel_.enemyHitPoints.push_back(enemyHP);
}

void GameUIController::OnEnemyScreenPosition(const EnemyScreenPositionEvent& event)
{
    for (auto& enemyHP : viewModel_.enemyHitPoints)
    {
        if (enemyHP.enemyID != event.enemyID)
        {
            continue;
        }

        enemyHP.screenPosition = event.screenPosition;
        enemyHP.isVisible = event.isVisible;
        return;
    }
}

void GameUIController::OnEnemyHPChanged(const EnemyHPChangeEvent& event)
{
    for (auto& enemyHP : viewModel_.enemyHitPoints)
    {
        if (enemyHP.enemyID != event.enemyID)
        {
            continue;
        }

        enemyHP.hitPoint.currentHitPoint = event.currentHitPoint;
        enemyHP.hitPoint.previousHitPoint = event.previousHitPoint;
        enemyHP.hitPoint.maxHitPoint = event.maxHitPoint;

        return;
    }
}

void GameUIController::OnNearestEnemyInfo(const NearestEnemyInfoEvent& event)
{
	// ロックオン中の表示位置は、選択済みの敵に固定する。
	if (!viewModel_.isLockOn)
	{
		viewModel_.targetEnemyWorldPosition = event.worldPosition;
	}
}

void GameUIController::OnPlayerLockOn(const PlayerLockOnEvent& event)
{
    viewModel_.isLockOn = event.isLockOn;
	viewModel_.targetEnemyID = event.isLockOn ? event.targetEnemyID : 0;
	if (event.isLockOn)
	{
		viewModel_.targetEnemyWorldPosition = event.targetWorldPosition;
	}
}
