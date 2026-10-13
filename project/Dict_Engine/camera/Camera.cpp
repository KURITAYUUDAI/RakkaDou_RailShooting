#include "Camera.h"
#include "WindowsAPI.h"

Camera::Camera()
	: fovY_(0.45f)
	, aspectRatio_(static_cast<float>(WindowsAPI::kClientWidth) / static_cast<float>(WindowsAPI::kClientHeight))
	, nearClip_(0.1f)
	, farClip_(300.0f)
	, viewMatrix_(MakeIdentity4x4())
	, projectionMatrix_(MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_))
	, viewProjectionMatrix_(MakeIdentity4x4())
	, billboardMatrix_(MakeIdentity4x4())
{}

void Camera::Initialize()
{	
	rotate_ = {0.0f, 0.0f, 0.0f};
	translate_ = {0.0f, 0.0f, 0.0f};
	// DebugCamera::Initialize() からも呼ばれるため、仮想関数呼び出しにはしない。
	// 基底Cameraの行列計算だけを行い、ImGuiを使うDebugCamera::Update()は
	// 通常のフレーム更新まで呼ばない。
	Camera::Update();
}

void Camera::Update()
{
	worldMatrix_ = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotate_, translate_);

	//worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

	viewMatrix_ = Inverse(worldMatrix_);

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);

	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(pi);
	billboardMatrix_ = Multiply(backToFrontMatrix, worldMatrix_);
	billboardMatrix_.m[3][0] = 0.0f;
	billboardMatrix_.m[3][1] = 0.0f;
	billboardMatrix_.m[3][2] = 0.0f;

	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

void Camera::Finalize()
{


}

const Matrix4x4 Camera::GetBillboardWorldMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) const
{
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	Matrix4x4 worldMatrix = scaleMatrix * rotateMatrix * billboardMatrix_ * translateMatrix;

	return worldMatrix;
}

const Vector3 Camera::GetCameraViewPosition() const
{
	Matrix4x4 invViewMatrix = Inverse(viewMatrix_);
	
	Vector3 cameraViewPosition = 
	{
		invViewMatrix.m[3][0],
		invViewMatrix.m[3][1],
		invViewMatrix.m[3][2]
	};

	return cameraViewPosition;
}
