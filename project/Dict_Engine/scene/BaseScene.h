#pragma once

#include "CanvasManager.h"

// シーン基底クラス
class BaseScene
{
public:
	virtual ~BaseScene() = default;

	virtual void Initialize() = 0;

	virtual void Finalize() = 0;

	virtual void Update(const float& deltaTime) = 0;

	virtual void FinishFadeIn() = 0;

	virtual void Draw(CanvasId canvasId) = 0;

};

