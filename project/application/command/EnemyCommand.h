#pragma once
#include "myMath.h"

class BossEnemy;

class AIHandler;

class IEnemyCommand
{
public:

	virtual ~IEnemyCommand() = default;
	virtual void Execute(BossEnemy* enemy) = 0;
};

class EnemyMoveCommand : public IEnemyCommand
{
public:
	EnemyMoveCommand(AIHandler* aiHandler) : aiHandler_(aiHandler) {}
	void Execute(BossEnemy* enemy) override;

private:
	AIHandler* aiHandler_;
};

class EnemyShotCommand : public IEnemyCommand
{
public:
	EnemyShotCommand(AIHandler* aiHandler) : aiHandler_(aiHandler) {}
	void Execute(BossEnemy* enemy) override;

private:
	AIHandler* aiHandler_;
};

class EnemyAttackCommand : public IEnemyCommand
{
public:
	EnemyAttackCommand(AIHandler* aiHandler) : aiHandler_(aiHandler) {}
	void Execute(BossEnemy* enemy) override;

private:
	AIHandler* aiHandler_;
};


