#pragma once
#include "ICameraController.h"
#include "SplineCurve.h"

class RailCameraController :public ICameraController
{
public:
	enum class Mode
	{
		Loop,
		VerticalDescent,
	};

	void Initialize() override;
	void Initialize(Mode mode);
	void Update(Camera* mainCamera, const float& deltaTime) override;
	void Finalize() override;
	void DrawDebugUI(const Camera* mainCamera, bool& isDebugCamera) override;

	void BuildLenghthTable();
	float GetTFromDistance(float distance);
	float GetTotalLength() const { return totalLength_; }
	float GetCurrentDistance() const { return currentDistance_; }
	float GetTravelledDistance() const { return travelledDistance_; }
	void SetLoop(bool isLoop) { isLoop_ = isLoop; }
	bool IsLoop() const { return isLoop_; }
	void SetMovementEnabled(bool enabled) { isMovementEnabled_ = enabled; }
	bool IsMovementEnabled() const { return isMovementEnabled_; }
	void SetMoveSpeed(float speed) { moveSpeed_ = speed < 0.0f ? 0.0f : speed; }
	float GetMoveSpeed() const { return moveSpeed_; }
	void ResetTravelledDistance() { travelledDistance_ = 0.0f; }
	void SetVerticalStartHeight(float height);
	Mode GetMode() const { return mode_; }

	WorldTransform* GetBattleWorldTransform() { return &battleWorldTransform_; }
	const WorldTransform* GetBattleWorldTransform() const { return &battleWorldTransform_; }
	void SetBattleSpaceParent(WorldTransform* parent);
	void RecenterBattleSpace(Vector3& playerLocalPosition, Vector3& enemyLocalPosition);

private:
	Mode mode_ = Mode::Loop;

	std::vector<Vector3> controlPoints_;

	Vector3 railPos_ = {0.0f, 0.0f, 0.0f};
	Vector3 target_ = {0.0f, 0.0f, 0.0f};

	std::vector<SplineSample> lengthTable_; // 距離逆引き用テーブル
	float totalLength_ = 0.0f;              // レール全体の総延長（長さ）

	float currentDistance_ = 0.0f;          // カメラが実際に進んだ表示上の距離
	float travelledDistance_ = 0.0f;        // 敵フェーズ進行用。ループしても巻き戻さない累積距離
	float verticalStartHeight_ = 0.0f;       // 垂直降下を開始するワールドY
	float moveSpeed_ = 5.0f;                // 移動速度 (1秒間に進むメートル数)
	float stageProgressSpeed_ = 1.0f;        // 垂直降下中の敵出現進行速度
	bool isMovementEnabled_ = true;
	bool isLoop_ = true;                    // 終端到達後に先頭へ戻って周回するか

	float targetRange_ = 0.5f;
	
	Vector2 offset_ = {1.0f, 1.0f};

	WorldTransform battleWorldTransform_;
};

