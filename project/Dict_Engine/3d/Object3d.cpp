#include "Object3d.h"
#include "Object3dManager.h"
#include "Model.h"
#include "ModelUtility.h"
#include "ModelManager.h"
#include "Camera.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "DistanceFogManager.h"

void Object3d::Initialize()
{

	object3dManager_ = Object3dManager::GetInstance();

	//CreateTransformationMatrixResource();

	// Transform変数を作る
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.SetRotate({0.0f, pi, 0.0f}); 
	worldTransform_.translate_ = {0.0f, 0.0f, 0.0f} ;

	/*std::unique_ptr<MaterialInstance> material = std::make_unique<MaterialInstance>();
	material->Initialize();
	material_.push_back(std::move(material));*/

	psoName_ = Object3dManager::GetInstance()->GetDefaultPsoName();
	
}

void Object3d::Update(const Matrix4x4* worldMatrix, const Matrix4x4* multiplyMatrix, bool applyRootNode)
{
	/*worldTransform_.scale_ = transform_.scale;
	worldTransform_.rotate_ = transform_.rotate;
	worldTransform_.translate_ = transform_.translate;*/

	worldTransform_.UpdateMatrix(worldMatrix);
	if (worldTransform_.parent_)
	{
		worldTransform_.worldMatrix_ *= worldTransform_.parent_->worldMatrix_;
	}

	if (multiplyMatrix)
	{
		Matrix4x4 multipliedMatrix = Multiply(model_->GetRootNode(0).localMatrix, *multiplyMatrix);

		worldTransform_.TransferMatrix(
			CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix(),
			&multipliedMatrix);
	} 
	else
	{
		if (applyRootNode)
		{
			worldTransform_.TransferMatrix(
				CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix(),
					&model_->GetRootNode(0).localMatrix);
		}
		else
		{
			worldTransform_.TransferMatrix(CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix());
		}
	}


	/*Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 worldViewProjectionMatrix;

	const Matrix4x4& viewProjectionMatrix = CameraManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix();
	worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	CameraManager::GetInstance()->SetCameraWorldPosition(CameraManager::GetInstance()->GetActiveCamera()->GetTranslate());

	transformationMatrixData_->World = worldMatrix;
	transformationMatrixData_->WVP = worldViewProjectionMatrix;*/
}

void Object3d::Draw(const D3D12_VERTEX_BUFFER_VIEW* additionalVBV, 
	const D3D12_GPU_DESCRIPTOR_HANDLE* additionalGPUHandle)
{
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// wvp用のCBufferの場所を設定
	worldTransform_.SetCBufferTransformationResource(1);

	// 平行光源用のCBufferをバインド（rootParameter[3] = b1）
	LightManager::GetInstance()->SetCBufferLightsResource(3);

	// カメラ用のCBufferをバインド（rootParameter[4] = b2）
	CameraManager::GetInstance()->SetCbufferCameraResource(4);

	// rootParameter[5] はPSOごとに用途が異なる。
	// 距離フォグCBVを持つ標準Object3d PSOでのみバインドする。
	// Environment PSOでは同じ番号が環境テクスチャのDescriptorTableになる。
	if (psoName_ == object3dManager_->GetDefaultPsoName())
	{
		DistanceFogManager::GetInstance()->SetCBufferResource(5);
	}
	else if (psoName_ == "Environment")
	{
		DistanceFogManager::GetInstance()->SetCBufferResource(8);
	}

	// 3Dモデルが割り当てられていれば描画する
	if (model_)
	{
		for (uint32_t index = 0; index < model_->GetMeshCount(); ++index)
		{
			// マテリアルとSRVをバインド
			material_[model_->GetMesh(index).materialIndex]->Draw(0, 2);
			model_->DrawMesh(index, 1, additionalVBV, additionalGPUHandle);
		}
	}
}

void Object3d::DrawWithOverrideVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView)
{
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// wvp用のCBufferの場所を設定
	worldTransform_.SetCBufferTransformationResource(1);

	// 平行光源用のCBufferをバインド（rootParameter[3] = b1）
	LightManager::GetInstance()->SetCBufferLightsResource(3);

	// カメラ用のCBufferをバインド（rootParameter[4] = b2）
	CameraManager::GetInstance()->SetCbufferCameraResource(4);

	// rootParameter[5] はPSOごとに用途が異なる。
	// 距離フォグCBVを持つ標準Object3d PSOでのみバインドする。
	// Environment PSOでは同じ番号が環境テクスチャのDescriptorTableになる。
	if (psoName_ == object3dManager_->GetDefaultPsoName())
	{
		DistanceFogManager::GetInstance()->SetCBufferResource(5);
	} else if (psoName_ == "Environment")
	{
		DistanceFogManager::GetInstance()->SetCBufferResource(8);
	}

	// 3Dモデルが割り当てられていれば描画する
	if (model_)
	{
		for (uint32_t index = 0; index < model_->GetMeshCount(); ++index)
		{
			// マテリアルとSRVをバインド
			material_[model_->GetMesh(index).materialIndex]->Draw(0, 2);
			model_->DrawMeshWithOverrideVertexBuffer(index, 1, vertexBufferView);
		}
	}
}

void Object3d::Finalize()
{

}

void Object3d::SetModel(const std::string& filePath)
{
	model_ = ModelManager::GetInstance()->FindModel(filePath);
	material_.clear();
	material_.resize(model_->GetMaterialAssetsCount());
	for (uint32_t index = 0; index < model_->GetMaterialAssetsCount(); ++index)
	{
		std::unique_ptr<MaterialInstance> material = std::make_unique<MaterialInstance>();
		material->Initialize();
		material->SetMaterialAsset(model_->GetMaterialAsset(index));
		material_[index] = std::move(material);
	}
}

void Object3d::SetEnableLighting(const int32_t& enableLighting, const uint32_t* materialIndex)
{
	if (material_.size() == 0)
	{
		return;
	}

	enableLighting_ = enableLighting;
	if (materialIndex != nullptr)
	{
		material_[*materialIndex]->SetEnableLighting(enableLighting_);
	}
	else
	{
		for (auto& material : material_)
		{
			material->SetEnableLighting(enableLighting_);
		}
	}
}

void Object3d::SetColor(const Vector4& color, const uint32_t* materialIndex)
{
	if (material_.size() == 0)
	{
		return;
	}

	color_ = color;
	if (materialIndex != nullptr)
	{
		material_[*materialIndex]->SetColor(color_);
	} 
	else
	{
		for (auto& material : material_)
		{
			material->SetColor(color_);
		}
	}
}

void Object3d::SetUVTransform(const EulerTransform& uvTransform, const uint32_t* materialIndex)
{
	if (material_.size() == 0)
	{
		return;
	}

	if (materialIndex != nullptr)
	{
		material_[*materialIndex]->SetUVTransform(uvTransform);
	} 
	else
	{
		for (auto& material : material_)
		{
			material->SetUVTransform(uvTransform);
		}
	}
}

void Object3d::SetShininess(const float& shininess, const uint32_t * materialIndex)
{
	if (material_.size() == 0)
	{
		return;
	}

	if (materialIndex != nullptr)
	{
		material_[*materialIndex]->SetShininess(shininess);
	} 
	else
	{
		for (auto& material : material_)
		{
			material->SetShininess(shininess);
		}
	}
}

void Object3d::SetEnvironmentCoefficient(const float& environmentCoefficient, const uint32_t * materialIndex)
{
	if (material_.size() == 0)
	{
		return;
	}

	if (materialIndex != nullptr)
	{
		material_[*materialIndex]->SetEnvironmentCoefficient(environmentCoefficient);
	} 
	else
	{
		for (auto& material : material_)
		{
			material->SetEnvironmentCoefficient(environmentCoefficient);
		}
	}
}

void Object3d::SetAlphaReference(const float alphaReference, const uint32_t * materialIndex)
{
	if (material_.size() == 0)
	{
		return;
	}

	if (materialIndex != nullptr)
	{
		material_[*materialIndex]->SetAlphaReference(alphaReference);
	} 
	else
	{
		for (auto& material : material_)
		{
			material->SetAlphaReference(alphaReference);
		}
	}
}

//void Object3d::CreateTransformationMatrixResource()
//{
//
//	// 座標変換行列リソースを作成する。Matrix4x4 1つ分のサイズを用意する
//	transformationMatrixResource_ = object3dManager_->GetDxBase()->CreateBufferResource(sizeof(TransformationMatrix));
//	// TransformationMatrixResourceにデータを書き込むためのアドレスを取得してTransformationMatrixDataに割り当てる
//	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
//	
//	// 座標変換行列データの初期値を書き込む
//	transformationMatrixData_->World = MakeIdentity4x4();
//	transformationMatrixData_->WVP = MakeIdentity4x4();
//
//}
