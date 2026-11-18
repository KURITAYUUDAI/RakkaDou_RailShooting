#include "GameUIController.h"
#include <unordered_map>


void GameUIController::Update(float deltaTime)
{
	(void)deltaTime;
}

void GameUIController::SyncSnapshot(const GameUIFrameSnapshot& snapshot)
{
	const int previousPlayerHP = hasSnapshot_
		? viewModel_.playerHitPoint.currentHitPoint
		: snapshot.playerHitPoint.currentHitPoint;
	viewModel_.playerHitPoint.currentHitPoint =
		snapshot.playerHitPoint.currentHitPoint;
	viewModel_.playerHitPoint.previousHitPoint = previousPlayerHP;
	viewModel_.playerHitPoint.maxHitPoint = snapshot.playerHitPoint.maxHitPoint;

	std::unordered_map<EnemyID, int> previousEnemyHP;
	previousEnemyHP.reserve(viewModel_.enemyHitPoints.size());
	for (const EnemyHPViewModel& enemy : viewModel_.enemyHitPoints)
	{
		previousEnemyHP[enemy.enemyID] = enemy.hitPoint.currentHitPoint;
	}

	viewModel_.enemyHitPoints.clear();
	viewModel_.enemyHitPoints.reserve(snapshot.enemies.size());
	for (const EnemyUIFrameSnapshot& enemy : snapshot.enemies)
	{
		EnemyHPViewModel enemyViewModel{};
		enemyViewModel.enemyID = enemy.enemyID;
		enemyViewModel.hitPoint.currentHitPoint = enemy.hitPoint.currentHitPoint;
		const auto previous = previousEnemyHP.find(enemy.enemyID);
		enemyViewModel.hitPoint.previousHitPoint =
			previous != previousEnemyHP.end()
			? previous->second
			: enemy.hitPoint.currentHitPoint;
		enemyViewModel.hitPoint.maxHitPoint = enemy.hitPoint.maxHitPoint;
		enemyViewModel.worldPosition = enemy.worldPosition;
		enemyViewModel.isVisible = enemy.isVisible;
		enemyViewModel.displayType = enemy.displayType;
		enemyViewModel.screenBossPriority = enemy.screenBossPriority;
		viewModel_.enemyHitPoints.push_back(enemyViewModel);
	}

	viewModel_.isLockOn = snapshot.isLockOn;
	viewModel_.targetEnemyID = snapshot.isLockOn ? snapshot.targetEnemyID : 0;
	viewModel_.targetEnemyWorldPosition = snapshot.isLockOn
		? snapshot.targetEnemyWorldPosition
		: Vector3{};
	hasSnapshot_ = true;
}
