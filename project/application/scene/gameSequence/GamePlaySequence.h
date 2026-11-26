#pragma once

#include "SequencePolicy.h"

enum class SequenceStatus
{
	Running,
	Finished,
};

class GamePlaySequence
{
public:
	virtual ~GamePlaySequence() = default;

	virtual void Start() = 0;
	virtual SequenceStatus Update(float deltaTime) = 0;
	virtual SequencePolicy GetPolicy() const = 0;

	virtual void DrawWorld() = 0;
	virtual void DrawOverlay() = 0;
	virtual void DrawUI() = 0;

	virtual void OnFadeInFinished()
	{
	}
};
