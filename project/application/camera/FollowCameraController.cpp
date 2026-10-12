#define NOMINMAX
#include "FollowCameraController.h"

#include "Camera.h"
#include "DebugDrawManager.h"
#include "ImGuiManager.h"

#include <algorithm>
#include <cmath>

void FollowCameraController::Initialize()
{
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.SetRotate({0.0f, 0.0f, 0.0f});
	worldTransform_.translate_ = followOffset_;
	worldTransform_.UpdateMatrix();
	hasSmoothedTargetPosition_ = false;
}

void FollowCameraController::Update(
	Camera* mainCamera, const float& deltaTime)
{
	if (!mainCamera || !target_)
	{
		return;
	}

#ifdef _DEBUG
	ImGui::Begin("FollowCamera");
	ImGui::DragFloat3("FollowOffset", &followOffset_.x, 0.1f);
	ImGui::DragFloat3("LookAtOffset", &lookAtOffset_.x, 0.1f);
	ImGui::DragFloat("FollowSharpness", &followSharpness_, 0.1f, 0.0f, 30.0f);
	ImGui::End();
#endif

	const Matrix4x4& targetWorldMatrix = target_->worldMatrix_;
	const Matrix4x4& orientationMatrix = orientationReference_
		? orientationReference_->worldMatrix_
		: targetWorldMatrix;
	const Vector3 targetPosition =
	{
		targetWorldMatrix.m[3][0],
		targetWorldMatrix.m[3][1],
		targetWorldMatrix.m[3][2]
	};

	// Playerの論理Rootを基準に、常に正面の反対側へカメラを配置する。
	// 回避の描画回転は論理Rootへ入らないため、カメラには伝わらない。
	const Vector3 right = Normalize(
		TransformNormal({1.0f, 0.0f, 0.0f}, orientationMatrix));
	const Vector3 up = Normalize(
		TransformNormal({0.0f, 1.0f, 0.0f}, orientationMatrix));
	const Vector3 forwardAxis = Normalize(
		TransformNormal({0.0f, 0.0f, 1.0f}, orientationMatrix));

	// 完成したカメラ位置を補間すると旋回時にPlayerを横切るため、
	// 追従中心だけを滑らかにし、向きに応じたオフセットは即時反映する。
	if (!hasSmoothedTargetPosition_)
	{
		smoothedTargetPosition_ = targetPosition;
		hasSmoothedTargetPosition_ = true;
	}
	else
	{
		const float followRate = 1.0f - std::exp(
			-followSharpness_ * std::max(deltaTime, 0.0f));
		smoothedTargetPosition_ +=
			(targetPosition - smoothedTargetPosition_) * followRate;
	}

	worldTransform_.translate_ = smoothedTargetPosition_
		+ right * followOffset_.x
		+ up * followOffset_.y
		+ forwardAxis * followOffset_.z;
	lookAtPosition_ = smoothedTargetPosition_
		+ right * lookAtOffset_.x
		+ up * lookAtOffset_.y
		+ forwardAxis * lookAtOffset_.z;

	Vector3 forward = lookAtPosition_ - worldTransform_.translate_;
	if (Length(forward) <= 0.00001f)
	{
		forward = {0.0f, 0.0f, 1.0f};
	}
	else
	{
		forward = Normalize(forward);
	}

	Vector3 rotate{};
	rotate.y = std::atan2(forward.x, forward.z);
	rotate.x = std::atan2(
		-forward.y, Length(Vector2{forward.x, forward.z}));

	worldTransform_.SetRotate(rotate);
	worldTransform_.UpdateMatrix();

	mainCamera->SetRotate(worldTransform_.GetRotate());
	mainCamera->SetTranslate(worldTransform_.translate_);
}

void FollowCameraController::Finalize()
{
	worldTransform_.Finalize();
	target_ = nullptr;
	orientationReference_ = nullptr;
}

void FollowCameraController::DrawDebugUI(
	const Camera* mainCamera, bool& isDebugCamera)
{
	(void)mainCamera;

#ifdef _DEBUG
	
	if (isDebugCamera && target_)
	{
		DebugDrawManager::GetInstance()->AddSphere(
			worldTransform_.translate_, 0.2f,
			{0.0f, 1.0f, 0.0f, 1.0f}, 12);
		DebugDrawManager::GetInstance()->AddSphere(
			lookAtPosition_, 0.15f,
			{0.0f, 1.0f, 1.0f, 1.0f}, 12);
		DebugDrawManager::GetInstance()->AddLine(
			worldTransform_.translate_, lookAtPosition_,
			{0.0f, 1.0f, 1.0f, 1.0f});
	}
#else
	(void)isDebugCamera;
#endif
}

void FollowCameraController::SetTarget(const WorldTransform* target)
{
	if (target_ != target)
	{
		hasSmoothedTargetPosition_ = false;
	}
	target_ = target;
}

void FollowCameraController::SetOrientationReference(
	const WorldTransform* reference)
{
	if (orientationReference_ != reference)
	{
		hasSmoothedTargetPosition_ = false;
	}
	orientationReference_ = reference;
}
