#define NOMINMAX
#include "DebugCamera.h"
#include "InputManager.h"
#include "ImGuiManager.h"

void DebugCamera::Initialize()
{
	Camera::Initialize();
	theta = 0.52f;
	phi = 0.0f;
}

void DebugCamera::Update()
{
#ifdef _DEBUG

	originRotate_ = rotate_;
	originTranslate_ = translate_;

	ImGui::Begin("DebugCameraWindow");

	if (InputManager::GetInstance()->PushMouse(1) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
	{
		// ウィンドウ上ではない領域でのドラッグ
		ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);

		// カメラ角度に反映
		phi += delta.x * rotationSpeed;
		theta += delta.y * rotationSpeed;

		if (phi > pi)
		{
			phi = phi - 2.0f * pi;
		} else if (phi < -pi)
		{
			phi = phi + 2.0f * pi;
		}

		// ピッチを上下90°未満にクランプ
		const float limit = 0.4999f * pi; // 約85°
		theta = std::clamp(theta, -limit, limit);

		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);

		ImGui::Text("Delta x : %3.6f, y : %3.6f", delta.x, delta.y);

	}

	LONG wheel = InputManager::GetInstance()->MouseWheel(); // 前フレームとの差分
	radius -= wheel * 0.005f;              // 感度は 0.1f などで調整
	radius = std::max(1.0f, std::min(radius, 20.0f)); // クランプして近づきすぎ防止
	ImGui::DragFloat(
		"Distance Offset", &distanceOffset_, 0.1f, -19.0f, 100.0f);
	const float effectiveRadius = std::max(1.0f, radius + distanceOffset_);

	rotate_.x = phi;
	rotate_.y = theta;

	translate_.x = target_->translate_.x + effectiveRadius * std::cos(theta) * std::sin(phi);
	translate_.y = target_->translate_.y + effectiveRadius * std::sin(theta);
	translate_.z = target_->translate_.z + effectiveRadius * std::cos(theta) * std::cos(phi);

	viewMatrix_ = MakeLookAtMatrix(translate_, target_->translate_, { 0.0f, 1.0f, 0.0f });

	ImGui::Text("Target Pos : X:%3.2f, Y:%3.2f, Z:%3.2f", 
		target_->translate_.x, target_->translate_.y, target_->translate_.z);
	ImGui::Text("Camera Distance : %3.2f", radius);
	ImGui::Text("Effective Distance : %3.2f", effectiveRadius);

	ImGui::End();

	worldMatrix_ = Inverse(viewMatrix_);

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);

	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(pi);
	billboardMatrix_ = Multiply(backToFrontMatrix, worldMatrix_);
	billboardMatrix_.m[3][0] = 0.0f;
	billboardMatrix_.m[3][1] = 0.0f;
	billboardMatrix_.m[3][2] = 0.0f;

	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

#endif
}
