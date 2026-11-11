#pragma once
#include "DirectXBase.h"
#include "object3d.h"
#include "myMath.h"
#include "Model.h"
#include <memory>
#include <vector>

class CaveRailPath;

class Terrain
{
public:
	enum class Mode
	{
		Gameplay,
		TitleCave,
		GameplayCave,
	};

	void Initialize(Mode mode = Mode::Gameplay, const CaveRailPath* caveRailPath = nullptr);
	void Update(float deltaTime);
	void Update(float deltaTime, const Vector3& referencePosition);
	void Update(
		float deltaTime,
		const Vector3& referencePosition,
		float railDistance,
		const Vector3& pathWorldOffset,
		const Matrix4x4& railFrameWorldMatrix);
	void Draw();
	void Finalize();
	void SetGameplayCaveScrolling(bool enabled);
	bool IsGameplayCaveScrolling() const { return isGameplayCaveScrolling_; }
	void SetCaveSpeed(float speed) { caveSpeed_ = speed < 0.0f ? 0.0f : speed; }
	void SetGameplayCaveEntryMode(bool enabled);
	float GetGameplayCaveForwardLength() const { return kCaveRecycleHalfDistance_; }

private:

	std::unique_ptr<Object3d> object3d_;
	std::vector<std::unique_ptr<Object3d>> caveObjects_;
	std::vector<EulerTransform> caveTransforms_;
	std::vector<bool> caveVisibility_;

	struct CrystalPlacement
	{
		float distance = 0.0f;
		float angle = 0.0f;
		float scale = 1.0f;
		float roll = 0.0f;
	};

	const CaveRailPath* caveRailPath_ = nullptr;
	std::vector<std::unique_ptr<Object3d>> crystalObjects_;
	std::vector<bool> crystalVisibility_;
	std::vector<CrystalPlacement> crystalPlacements_;
	float gameplayCaveViewerDistance_ = 0.0f;
	float gameplayCavePathOffsetY_ = 0.0f;
	Matrix4x4 gameplayCaveScrollAnchor_{};
	bool isGameplayCaveScrollAnchorInitialized_ = false;

	void InitializeGameplayCave();
	void BuildContinuousCaveModel();
	void UpdateGameplayCave(
		float deltaTime,
		const Vector3& referencePosition,
		float railDistance,
		const Vector3& pathWorldOffset,
		const Matrix4x4& railFrameWorldMatrix);
	void EnsureCrystalPlacements(float requiredDistance);
	Matrix4x4 MakeDisplayMatrix(const Matrix4x4& sourceWorld, float viewerDistance) const;

	EulerTransform transform_;
	Mode mode_ = Mode::Gameplay;
	float caveSpeed_ = 100.0f;
	bool isGameplayCaveScrolling_ = false;
	bool isGameplayCaveEntryMode_ = false;
	static constexpr size_t kCaveObjectCount_ = 20;
	static constexpr float kCaveSegmentLength_ = 50.0f;
	static constexpr float kCaveRecycleDistance_ =
		kCaveSegmentLength_ * static_cast<float>(kCaveObjectCount_);
	static constexpr float kCaveRecycleHalfDistance_ = kCaveRecycleDistance_ * 0.5f;
	static constexpr float kOutFieldBoundingRadius_ = 55.3f;
	static constexpr float kOutFieldVisibilityMargin_ = kCaveSegmentLength_;

	bool isDraw_ = true;
};

