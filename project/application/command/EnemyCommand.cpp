#include "EnemyCommand.h"
#include "Enemy/BossEnemy.h"
#include "AIHandler.h"

void EnemyMoveCommand::Execute(BossEnemy* enemy)
{
    Vector2 direction = aiHandler_->GetDirection();
    if (direction.x == 0.0f && direction.y == 0.0f)
    {
        enemy->Decelerate(); // 入力なし → 減速
    } 
    else
    {
        enemy->Move(direction.x, direction.y); // 入力あり → 加速
    }
}

void EnemyShotCommand::Execute(BossEnemy* enemy)
{
    enemy->Shot();
}

void EnemyAttackCommand::Execute(BossEnemy* enemy)
{
    enemy->Attack();
}
