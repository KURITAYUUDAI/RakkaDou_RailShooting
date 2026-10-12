#pragma once

#include "ICameraController.h"

class RailRoot;

class RailCameraController : public ICameraController
{
public:
	void Initialize() override;
	void Update(Camera* mainCamera, const float& deltaTime) override;
	void Finalize() override;
	void DrawDebugUI(
		const Camera* mainCamera, bool& isDebugCamera) override;

	void SetRailRoot(const RailRoot* railRoot) { railRoot_ = railRoot; }
	const RailRoot* GetRailRoot() const { return railRoot_; }

private:
	const RailRoot* railRoot_ = nullptr;
	Vector2 offset_ = {1.0f, 1.0f};
};
