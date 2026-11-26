#pragma once

enum class TitleSequenceStatus
{
	Running,
	Finished,
};

enum class TitleSequenceCompletionAction
{
	BeginExit,
	BeginGameplay,
};

class TitleSequence
{
public:
	virtual ~TitleSequence() = default;

	virtual void Start() = 0;
	virtual TitleSequenceStatus Update(float deltaTime) = 0;
	virtual bool HandleConfirm() = 0;
	virtual TitleSequenceCompletionAction GetCompletionAction() const = 0;
	virtual void DrawOverlay(bool drawTitle) = 0;
};
