#pragma once
#include "../math/myMath.h"
#include <Windows.h>
#include "Camera.h"

/// <summary>
///  デバッグカメラ
/// </summary>
class DebugCamera : public Camera
{
public:

	/// <summary>
	///  初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	///  更新
	/// </summary>
	void Update() override;

	void SetTargetPosition(WorldTransform* target) { target_ = target; }

private:

	Vector3 originRotate_;
	Vector3 originTranslate_;

	WorldTransform* target_ = nullptr;

	float radius = 6.0f;    // 原点からの距離
	float distanceOffset_ = 0.0f;
	const float rotationSpeed = 0.005f; // ドラッグ速度の調整係数

	float theta = 0.0f;
	float phi = 0.0f;

	// マウス座標を取得
	int mouseX, mouseY;

};
