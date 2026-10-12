#define NOMINMAX
#include "RailCameraController.h"

#include "Camera.h"
#include "DebugDrawManager.h"
#include "ImGuiManager.h"
#include "RailRoot.h"

void RailCameraController::Initialize()
{
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.SetRotate({0.0f, 0.0f, 0.0f});
	worldTransform_.translate_ = {};
	worldTransform_.UpdateMatrix();
}

void RailCameraController::Update(
	Camera* mainCamera, const float& deltaTime)
{
	(void)deltaTime;
	if (!mainCamera || !railRoot_)
	{
		return;
	}

	const WorldTransform* railTransform = railRoot_->GetWorldTransform();
	const Matrix4x4& railWorldMatrix = railTransform->worldMatrix_;
	const Vector3 railPosition =
	{
		railWorldMatrix.m[3][0],
		railWorldMatrix.m[3][1],
		railWorldMatrix.m[3][2]
	};
	const Vector3 right = Normalize(
		TransformNormal({1.0f, 0.0f, 0.0f}, railWorldMatrix));
	const Vector3 up = Normalize(
		TransformNormal({0.0f, 1.0f, 0.0f}, railWorldMatrix));

	worldTransform_.translate_ =
		railPosition + right * offset_.x + up * offset_.y;
	worldTransform_.SetRotate(railTransform->GetRotate());
	worldTransform_.UpdateMatrix();

	mainCamera->SetRotate(worldTransform_.GetRotate());
	mainCamera->SetTranslate(worldTransform_.translate_);
}

void RailCameraController::Finalize()
{
	worldTransform_.Finalize();
	railRoot_ = nullptr;
}

void RailCameraController::DrawDebugUI(
	const Camera* mainCamera, bool& isDebugCamera)
{
	(void)mainCamera;

#ifdef _DEBUG
	ImGui::Begin("RailCamera");
	ImGui::DragFloat2("Offset", &offset_.x, 0.1f);
	ImGui::End();

	if (isDebugCamera && railRoot_)
	{
		const Matrix4x4& railWorldMatrix =
			railRoot_->GetWorldTransform()->worldMatrix_;
		const Vector3 railPosition =
		{
			railWorldMatrix.m[3][0],
			railWorldMatrix.m[3][1],
			railWorldMatrix.m[3][2]
		};
		DebugDrawManager::GetInstance()->AddSphere(
			worldTransform_.translate_, 0.2f,
			{0.0f, 1.0f, 0.0f, 1.0f}, 12);
		DebugDrawManager::GetInstance()->AddLine(
			worldTransform_.translate_, railPosition,
			{0.0f, 0.0f, 1.0f, 1.0f});
	}
#else
	(void)isDebugCamera;
#endif
}
