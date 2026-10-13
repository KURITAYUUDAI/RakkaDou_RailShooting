#include "WorldTransform.h"
#include "CameraManager.h"

uint32_t WorldTransform::currentFrame_ = 0;

void WorldTransform::Initialize()
{
	CreateTransformationMatrixResource();
	// Update() 前に親行列や座標取得から参照されてもゼロ行列にならないよう、
	// メンバーの初期値から有効なワールド行列を作っておく。
	UpdateMatrix();

	lastUpdateFrame_ = 0;
}

void WorldTransform::UpdateMatrix(const Matrix4x4* worldMatrix)
{
	if (worldMatrix)
	{
		worldMatrix_ = *worldMatrix;
		return;
	}
	
	rotateQuat_ = Normalize(rotateQuat_);
	worldMatrix_ = MakeAffineMatrix(scale_, rotateQuat_, translate_);
}

void WorldTransform::TransferMatrix(const Matrix4x4& viewProjection, const Matrix4x4* worldMatrixMultiply)
{
	if (worldMatrixMultiply)
	{
		transformationMatrixData_->World = *worldMatrixMultiply * worldMatrix_;
		transformationMatrixData_->WVP = *worldMatrixMultiply * Multiply(worldMatrix_, viewProjection);
		return;
	}
	
	transformationMatrixData_->World = worldMatrix_;
	transformationMatrixData_->WVP = Multiply(worldMatrix_, viewProjection);
}

//void WorldTransform::UpdateTransform()
//{
//	
//	if (lastUpdateFrame_ == currentFrame_) return;
//
//	if (parent_)
//	{
//		const_cast<WorldTransform*>(parent_)->UpdateTransform();
//	}
//
//	worldMatrix_ = MakeAffineMatrixB(scale_, rotate_, translate_);
//
//	if (parent_)
//	{
//		worldMatrix_ = worldMatrix_ * parent_->worldMatrix_;
//	}
//
//	transformationMatrixData_->World = worldMatrix_;	
//
//	lastUpdateFrame_ = currentFrame_;
//}
//
//void WorldTransform::UpdateWVP(const Matrix4x4& viewProjection)
//{
//	transformationMatrixData_->WVP = Multiply(worldMatrix_, viewProjection);
//}

void WorldTransform::Finalize()
{
	
}

void WorldTransform::SetCBufferTransformationResource(UINT rootParamaterIndex)
{
	// wvp用のCBufferの場所を設定
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(rootParamaterIndex, transformationMatrixResource_->GetGPUVirtualAddress());
}

void WorldTransform::SetRotate(const Vector3& rotate)
{
	rotate_ = rotate;
	rotateQuat_ = MakeFromEuler(rotate);
}

void WorldTransform::SetRotateQuat(const Quaternion& rotateQuat)
{
	rotateQuat_ = rotateQuat;
	rotate_ = MakeToEuler(rotateQuat);
}

void WorldTransform::CreateTransformationMatrixResource()
{
	// 座標変換行列リソースを作成する。Matrix4x4 1つ分のサイズを用意する
	transformationMatrixResource_ = DirectXBase::GetInstance()->CreateConstantBufferResource(sizeof(TransformationMatrix));
	// TransformationMatrixResourceにデータを書き込むためのアドレスを取得してTransformationMatrixDataに割り当てる
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	// 座標変換行列データの初期値を書き込む
	transformationMatrixData_->World = MakeIdentity4x4();
	transformationMatrixData_->WVP = MakeIdentity4x4();
}
