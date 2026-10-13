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
	};

	void Initialize(Mode mode = Mode::Gameplay);
	void Update(float deltaTime);
	void Draw();
	void Finalize();

private:

	std::unique_ptr<Object3d> object3d_;
	std::vector<std::unique_ptr<Object3d>> caveObjects_;
	std::vector<EulerTransform> caveTransforms_;

	EulerTransform transform_;
	Mode mode_ = Mode::Gameplay;
	float caveSpeed_ = 100.0f;
	static constexpr size_t kCaveObjectCount_ = 20;
	static constexpr float kCaveSegmentLength_ = 50.0f;
	static constexpr float kCaveRecycleDistance_ =
		kCaveSegmentLength_ * static_cast<float>(kCaveObjectCount_);

	bool isDraw_ = true;
};

