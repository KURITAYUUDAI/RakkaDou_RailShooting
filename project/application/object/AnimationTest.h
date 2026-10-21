#pragma once
#include "object3d.h"
#include "myMath.h"
#include "Model.h"
#include <memory>

#include <vector>

#include "AnimationUtility.h"

class AnimationTest
{
public:
	void Initialize();
	void Update(float deltaTime);
	void Draw();
	void Finalize();

public: // 外部入出力

	// セッター
	void SetPsoName(const std::string& psoName){ psoName_ = psoName; }
	void SetBlendMode(const PSOManager::BlendMode& blendMode){ blendMode_ = blendMode; }
	void SetFillMode(const PSOManager::FillMode& fillMode){ fillMode_ = fillMode; }
	
	void SetScale(const Vector3& scale){ transform_.scale = scale; }
	void SetRotate(const Vector3& rotate){ transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate){ transform_.translate = translate; }

	// ゲッター
	const std::string& GetPsoName() const { return psoName_; }
	const PSOManager::BlendMode& GetBlendMode() const { return blendMode_; }
	const PSOManager::FillMode& GetFillMode() const { return fillMode_; }

	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }


private:

	std::string psoName_ = "Skinning";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	std::string computePsoName_ = "Skinning";

	std::unique_ptr<Object3d> object3d_;

	uint32_t uavIndex_ = 0;

	Animation animation_;
	float animationTime = 0.0f;
	Skeleton skeleton_;
	SkinCluster skinCluster_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> skinClusterHeap_;

	EulerTransform transform_;
};

