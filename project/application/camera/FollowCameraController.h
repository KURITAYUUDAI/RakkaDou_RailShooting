#pragma once

#include "ICameraController.h"

class FollowCameraController : public ICameraController
{
public:
	void Initialize() override;
	void Update(Camera* mainCamera, const float& deltaTime) override;
	void Finalize() override;
	void DrawDebugUI(
		const Camera* mainCamera, bool& isDebugCamera) override;

	void SetTarget(const WorldTransform* target);
	void SetOrientationReference(const WorldTransform* reference);
	const WorldTransform* GetTarget() const { return target_; }
	const WorldTransform* GetOrientationReference() const
	{
		return orientationReference_;
	}

private:
	const WorldTransform* target_ = nullptr;
	const WorldTransform* orientationReference_ = nullptr;

	Vector3 followOffset_ = {0.0f, 2.0f, -15.0f};
	Vector3 lookAtOffset_ = {0.0f, 1.0f, 3.0f};
	Vector3 smoothedTargetPosition_ = {};
	Vector3 lookAtPosition_ = {};
	float followSharpness_ = 8.0f;
	bool hasSmoothedTargetPosition_ = false;
};
