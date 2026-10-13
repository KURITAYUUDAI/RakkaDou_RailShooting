#pragma once
#include "DirectXBase.h"
#include "object3d.h"
#include "myMath.h"
#include "Model.h"
#include <memory>
#include <vector>

class Terrain
{
public:
	enum class Mode
	{
		Gameplay,
		TitleCave,
		GameplayCave,
	};

	void Initialize(Mode mode = Mode::Gameplay);
	void Update(float deltaTime);
	void Update(float deltaTime, const Vector3& referencePosition);
	void Draw();
	void Finalize();
	void SetGameplayCaveScrolling(bool enabled) { isGameplayCaveScrolling_ = enabled; }
	bool IsGameplayCaveScrolling() const { return isGameplayCaveScrolling_; }

private:

	std::unique_ptr<Object3d> object3d_;
	std::vector<std::unique_ptr<Object3d>> caveObjects_;
	std::vector<EulerTransform> caveTransforms_;
	std::vector<bool> caveVisibility_;

	EulerTransform transform_;
	Mode mode_ = Mode::Gameplay;
	float caveSpeed_ = 100.0f;
	bool isGameplayCaveScrolling_ = false;
	static constexpr size_t kCaveObjectCount_ = 20;
	static constexpr float kCaveSegmentLength_ = 50.0f;
	static constexpr float kCaveRecycleDistance_ =
		kCaveSegmentLength_ * static_cast<float>(kCaveObjectCount_);
	static constexpr float kCaveRecycleHalfDistance_ = kCaveRecycleDistance_ * 0.5f;
	static constexpr float kOutFieldBoundingRadius_ = 55.3f;

	bool isDraw_ = true;
};

