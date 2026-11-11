#pragma once
#include "myMath.h"
#include "WorldTransform.h"

class Camera
{
public:	// メンバ関数

	Camera();

	virtual ~Camera() = default;

	virtual void Initialize();

	virtual void Update();

	/*void TransformView();

	void Transformation();*/

	void Finalize();

public:	// 外部入出力

	// セッター
	/*void SetScale(const Vector3& scale){ transform_.scale = scale; }*/
	void SetRotate(const Vector3& rotate)
	{
		rotate_ = rotate;
		rotateQuat_ = MakeFromEuler(rotate);
	}
	void SetRotateQuat(const Quaternion& rotateQuat)
	{
		rotateQuat_ = Normalize(rotateQuat);
		rotate_ = MakeToEuler(rotateQuat_);
	}
	void SetTranslate(const Vector3& translate){ translate_ = translate; }
	void SetFovY(const float& fovY){ fovY_ = fovY; }
	void SetAspectRatio(const float& aspectRatio){ aspectRatio_ = aspectRatio; }
	void SetNearClip(const float& nearClip){ nearClip_ = nearClip; }
	void SetFarClip(const float& farClip){ farClip_ = farClip; }

	void SetViewMatrix(const Matrix4x4& viewMatrix){ viewMatrix_ = viewMatrix; }


	// ゲッター
	/*const Vector3& GetScale() const { return transform_.scale; }*/
	const Vector3& GetRotate() const { return rotate_; }
	const Quaternion& GetRotateQuat() const { return rotateQuat_; }
	Vector3 GetForward() const
	{
		return Normalize(RotateVector({ 0.0f, 0.0f, 1.0f }, rotateQuat_));
	}
	Vector3 GetRight() const
	{
		return Normalize(RotateVector({ 1.0f, 0.0f, 0.0f }, rotateQuat_));
	}
	Vector3 GetUp() const
	{
		return Normalize(RotateVector({ 0.0f, 1.0f, 0.0f }, rotateQuat_));
	}
	const Vector3& GetTranslate() const { return translate_; }
	const Matrix4x4& GetBillboardMatrix() const { return billboardMatrix_; }	
	const float& GetFovY() const { return fovY_; }
	const float& GetAspectRatio() const { return aspectRatio_; }
	const float& GetNearClip() const { return nearClip_; }
	const float& GetFarClip() const { return farClip_; }

	const EulerTransform& GetTransform() const 
	{ 
		return EulerTransform({{0.0f, 0.0f, 0.0f}, rotate_, translate_ });
	}

	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

	const Matrix4x4 GetBillboardWorldMatrix(const Vector3& scale,  const Vector3& rotate, const Vector3& translate) const;

	const Vector3 GetCameraViewPosition() const;

protected:	// メンバ変数

	Vector3 rotate_;
	Quaternion rotateQuat_ = IdentityQuaternion();
	Vector3 translate_;

	Matrix4x4 worldMatrix_;
	Matrix4x4 viewMatrix_;

	Matrix4x4 projectionMatrix_;
	float fovY_;
	float aspectRatio_;
	float nearClip_;
	float farClip_;

	Matrix4x4 viewProjectionMatrix_;

	Matrix4x4 billboardMatrix_;
	

};

