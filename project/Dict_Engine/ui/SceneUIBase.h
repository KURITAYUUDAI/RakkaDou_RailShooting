#pragma once
#include <memory>
#include "UIUtility.h"
#include "UIManager.h"

class SceneUIBase
{
public:
	virtual ~SceneUIBase() = default;
	virtual void Initialize() = 0;
	virtual void Finalize() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void DrawLayer(const UILayer& layer) = 0;
	virtual void SetIsVisible(bool visible) = 0;
};
