#pragma once
#include "myMath.h"
#include "DirectXBase.h"
#include "Object3dManager.h"
#include "PSOManager.h"
#include "WorldTransform.h"
#include "MaterialInstance.h"

class Model;

class Camera;

class Object3d
{

public:

	struct TransformationMatrix
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

public:	// メンバ関数
	
	void Initialize();

	void Update(const Matrix4x4* worldMatrix = nullptr, 
		const Matrix4x4* multiplyMatrix = nullptr, bool applyRootNode = true);

	void DrawingCommon();

	void Draw(const D3D12_VERTEX_BUFFER_VIEW* additionalVBV = nullptr, 
		const D3D12_GPU_DESCRIPTOR_HANDLE* additionalGPUHandle = nullptr);

	void DrawWithOverrideVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView);

	void Finalize();

public:	// 外部入出力

	// セッター
	void SetPsoName(const std::string& psoName){ psoName_ = psoName; }
	void SetBlendMode(const PSOManager::BlendMode& blendMode){ blendMode_ = blendMode; }
	void SetFillMode(const PSOManager::FillMode& fillMode){ fillMode_ = fillMode; }

	void SetModel(const std::string& filePath);
	void SetModel(Model* model){ model_ = model; }
	void SetCamera(Camera* camera){ camera_ = camera; }

	void SetScale(const Vector3& scale){ worldTransform_.scale_ = scale; }
	void SetRotate(const Vector3& rotate){ worldTransform_.SetRotate(rotate); }
	void SetTranslate(const Vector3& translate){ worldTransform_.translate_ = translate; }
	void SetTransform(const EulerTransform& transform)
	{ 
		worldTransform_.scale_ = transform.scale;
		worldTransform_.SetRotate(transform.rotate);
		worldTransform_.translate_ = transform.translate;
	}

	void SetEnableLighting(const int32_t& enableLighting, const uint32_t* materialIndex = nullptr);
	void SetColor(const Vector4& color, const uint32_t* materialIndex = nullptr);
	void SetUVTransform(const EulerTransform& uvTransform, const uint32_t* materialIndex = nullptr);
	void SetShininess(const float& shiniess, const uint32_t* materialIndex = nullptr);
	void SetEnvironmentCoefficient(const float& environmentCoefficient, const uint32_t* materialIndex = nullptr);
	void SetAlphaReference(const float alphaReference, const uint32_t* materialIndex = nullptr);

	void SetParent(WorldTransform* worldTransform){ worldTransform_.parent_ = worldTransform; }

	// ゲッター
	const std::string& GetPsoName() const { return psoName_; }
	const PSOManager::BlendMode& GetBlendMode() const { return blendMode_; }
	const PSOManager::FillMode& GetFillMode() const { return fillMode_; }

	const Vector3& GetScale() const { return worldTransform_.scale_; }
	const Vector3& GetRotate() const { return worldTransform_.GetRotate(); }
	const Vector3& GetTranslate() const { return worldTransform_.translate_; }
	const EulerTransform& GetTransform() const 
	{ 
		return EulerTransform(worldTransform_.scale_, worldTransform_.GetRotate(), worldTransform_.translate_);
	}
	WorldTransform* GetWorldTransform() { return &worldTransform_; }

	const int32_t& GetEnableLighting() const { return enableLighting_; }
	const Vector4& GetColor() const { return color_; }
	
	Model* GetModel() const { return model_; }
	MaterialInstance* GetMaterial(const size_t& index) const { return material_[index].get(); }

private: // 静的関数

	// TransformationMatrixResourceを作成
	/*void CreateTransformationMatrixResource();*/

private:

	std::string psoName_;
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	// 3Dオブジェクト共通処理
	Object3dManager* object3dManager_ = nullptr;

	// モデル
	Model* model_ = nullptr;

	// マテリアル
	std::vector<std::unique_ptr<MaterialInstance>> material_;

	// カメラ
	Camera* camera_ = nullptr;

	//// 座標変換行列用バッファリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
	//// バッファリソース内のデータを指すポインタ
	//TransformationMatrix* transformationMatrixData_ = nullptr;

	// トランスフォーム
	WorldTransform worldTransform_;

	// カラー
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// ライティング有効無効
	int32_t enableLighting_ = true;
};

