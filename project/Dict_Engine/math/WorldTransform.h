#pragma once
#include "DirectXBase.h"
#include "myMath.h"
#include "Quaternion.h"

class WorldTransform
{
public:

	struct TransformationMatrix
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

public:

	void Initialize();

	void UpdateMatrix(const Matrix4x4* worldMatrix = nullptr);
	void TransferMatrix(const Matrix4x4& viewProjection, const Matrix4x4* worldMatrixMultiply = nullptr);

	void Finalize();

	void SetCBufferTransformationResource(UINT rootParamaterIndex);

	static void AdvanceFrame() { ++currentFrame_; }

public:

	void SetScale(const Vector3& scale) { scale_ = scale; }
	void SetTranslate(const Vector3& translate) { translate_ = translate; }
	void SetRotate(const Vector3& rotate);
	void SetRotateQuat(const Quaternion& rotateQuat);
	void SetParent(const WorldTransform* parent) { parent_ = parent; }
	void SetWorldMatrix(const Matrix4x4& worldMatrix) { worldMatrix_ = worldMatrix; }

	const Vector3& GetScale() const { return scale_; }
	const Vector3& GetTranslate() const { return translate_; }
	const Vector3& GetRotate() const { return rotate_; }
	const Quaternion& GetRotateQuat() const { return rotateQuat_; }
	const WorldTransform* GetParent() const { return parent_; }
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

private:

	void CreateTransformationMatrixResource();

private:

	Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
	Quaternion rotateQuat_ = { 0.0f, 0.0f, 0.0f, 1.0f };

	Vector3 scale_ = {1.0f, 1.0f, 1.0f};
	Vector3 translate_{ 0.0f, 0.0f, 0.0f };

	Matrix4x4 worldMatrix_;

	const WorldTransform* parent_ = nullptr;

	static uint32_t currentFrame_;

	// 座標変換行列用バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData_;

	uint32_t lastUpdateFrame_ = 0;
};

