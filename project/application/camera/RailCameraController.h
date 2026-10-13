#pragma once
#include "ICameraController.h"
#include "SplineCurve.h"

class RailCameraController :public ICameraController
{
public:

	void Initialize() override;
	void Update(Camera* mainCamera, const float& deltaTime) override;
	void Finalize() override;
	void DrawDebugUI(const Camera* mainCamera, bool& isDebugCamera) override;

	void BuildLenghthTable();
	float GetTFromDistance(float distance);
	float GetTotalLength() const { return totalLength_; }
	float GetCurrentDistance() const { return currentDistance_; }
	float GetTravelledDistance() const { return travelledDistance_; }

	WorldTransform* GetBattleWorldTransform() { return &battleWorldTransform_; }
	const WorldTransform* GetBattleWorldTransform() const { return &battleWorldTransform_; }
	void SetBattleSpaceParent(WorldTransform* parent);
	void RecenterBattleSpace(Vector3& playerLocalPosition, Vector3& enemyLocalPosition);

private:

	std::vector<Vector3> controlPoints_;

	Vector3 railPos_ = {0.0f, 0.0f, 0.0f};
	Vector3 target_ = {0.0f, 0.0f, 0.0f};

	std::vector<SplineSample> lengthTable_; // 距離逆引き用テーブル
	float totalLength_ = 0.0f;              // レール全体の総延長（長さ）

	float currentDistance_ = 0.0f;          // 【重要】progress_の代わりに「進んだ距離(メートル)」で管理する
	float travelledDistance_ = 0.0f;        // フェーズ内の出現判定用。ループしても巻き戻さない累積距離
	float moveSpeed_ = 5.0f;                // 移動速度 (1秒間に進むメートル数)

	float targetRange_ = 0.5f;
	
	Vector2 offset_ = {1.0f, 1.0f};

	WorldTransform battleWorldTransform_;
};

