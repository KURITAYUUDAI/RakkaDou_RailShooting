#pragma once
#include "object3d.h"
#include "myMath.h"
#include "Model.h"
#include <memory>

#include <vector>
#include "../tool/effect/DissolveManager.h"

#include "Animation/AnimationUtility.h"
#include "Animation/AnimationPlayer.h"
#include "Animation/AnimationMemo.h"

class AnimationTest
{
public:

	struct SkinningInformation
	{
		uint32_t numVertices;
	};

public:
	void Initialize();
	void Update(float deltaTime);
	void Draw();
	void Finalize();

	void CreateInputVerticesSRV();
	void CreateInfluenceSRV();
	void CreateOutputVertexBuffer();
	void CreateSkinningInfoResource();
	void DispatchSkinning();

	void TransitionOutputVertex(D3D12_RESOURCE_STATES nextState);

public: // 外部入出力

	// セッター
	void SetPsoName(const std::string& psoName){ psoName_ = psoName; }
	void SetBlendMode(const PSOManager::BlendMode& blendMode){ blendMode_ = blendMode; }
	void SetFillMode(const PSOManager::FillMode& fillMode){ fillMode_ = fillMode; }
	
	void SetScale(const Vector3& scale){ transform_.scale = scale; }
	void SetRotate(const Vector3& rotate){ transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate){ transform_.translate = translate; }

	void SetEnvironmentTextureIndex(const uint32_t& index) { environmentTextureIndex_ = index; }

	void SetThreshold(const float threshold) { dissolveParams_.threshold = threshold; }
	void SetEdgeColor(const Vector4 edgeColor) { dissolveParams_.edgeColor = edgeColor; }

	// ゲッター
	const std::string& GetPsoName() const { return psoName_; }
	const PSOManager::BlendMode& GetBlendMode() const { return blendMode_; }
	const PSOManager::FillMode& GetFillMode() const { return fillMode_; }

	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }

	const float& GetThreshold() { return dissolveParams_.threshold; }
	const Vector4& GetEdgeColor() { return dissolveParams_.edgeColor; }


private:

	std::string psoName_ = "Environment";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	std::string computePsoName_ = "Skinning";

	std::unique_ptr<Object3d> object3d_;

	uint32_t environmentTextureIndex_ = 0;

	DissolveParams dissolveParams_;

	AnimationSet animationSet_;
	AnimationPlayer animationPlayer_;

	Skeleton skeleton_;
	SkinCluster skinCluster_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> skinClusterHeap_;
	uint32_t inputVerticesSrvIndex_ = 0;
	uint32_t influenceSrvIndex_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> outputVertexResource_;
	D3D12_VERTEX_BUFFER_VIEW outputVertexBufferView;
	uint32_t outputUavIndex_ = 0;
	D3D12_RESOURCE_STATES outputVertexState_ = D3D12_RESOURCE_STATE_COMMON;

	Microsoft::WRL::ComPtr<ID3D12Resource> skinningInfoResource_;
	SkinningInformation* skinningInfoData_ = nullptr;

	EulerTransform transform_;
};

