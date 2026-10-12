#pragma once

#include "SplineCurve.h"
#include "WorldTransform.h"

class RailRoot
{
public:
	void Initialize();
	void Update(float deltaTime);
	void Finalize();
	void DrawDebugUI();

	WorldTransform* GetWorldTransform() { return &worldTransform_; }
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }

private:
	void BuildLengthTable();
	float GetTFromDistance(float distance) const;
	void UpdateTransforms();

	std::vector<Vector3> controlPoints_;
	std::vector<SplineSample> lengthTable_;

	Vector3 railPosition_{};
	Vector3 targetPosition_{};

	float totalLength_ = 0.0f;
	float currentDistance_ = 0.0f;
	float moveSpeed_ = 1.0f;
	float targetRange_ = 0.5f;

	WorldTransform worldTransform_;
};
