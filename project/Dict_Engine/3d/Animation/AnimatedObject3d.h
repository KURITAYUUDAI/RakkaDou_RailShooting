#pragma once
#include "object3d.h"
#include "Animation/AnimationUtility.h"
#include "Animation/AnimationPlayer.h"
#include "Animation/Skeleton.h"
#include "Animation/MeshSkin.h"
#include "Model.h"

struct AnimatedModelAsset
{
	std::string modelName;
	// ModelManagerが所有するので非所有ポインタ
	Model* model = nullptr;
	AnimationSet animationSet;
	SkeletonAsset skeletonAsset;
	std::vector<std::optional<MeshSkinBinding>> skinBindings;
};

std::shared_ptr<AnimatedModelAsset> CreateAnimatedModelAsset(
	const std::string& directoryPath, const std::string& filename);

class AnimatedObject3d
{
public:
    void Initialize(std::shared_ptr<const AnimatedModelAsset> asset);

    void Update(float deltaTime);

    // Compute DispatchとObject3d::PreDraw
    void PreDraw();

    // メッシュごとの描画
    void Draw();

    void PlayAnimation(uint32_t animationIndex, bool isLoop = true);

    void PlayAnimation(const std::string& animationName, bool isLoop = true);

    void StopAnimation();
    void PauseAnimation();
    void ResumeAnimation();

public:	// 外部入出力
	// セッター
	void SetScale(const Vector3& scale) { object3d_.SetScale(scale); }
	void SetRotate(const Vector3& rotate) { object3d_.SetRotate(rotate); }
	void SetTranslate(const Vector3& translate) { object3d_.SetTranslate(translate); }
	void SetTransform(const EulerTransform& transform) { object3d_.SetTransform(transform); }

	void SetPsoName(const std::string& psoName) { object3d_.SetPsoName(psoName); }

	void SetBlendMode(PSOManager::BlendMode blendMode) { object3d_.SetBlendMode(blendMode); }

	void SetFillMode(PSOManager::FillMode fillMode) { object3d_.SetFillMode(fillMode); }

	void SetModel(const std::string& filePath) { object3d_.SetModel(filePath); }
	void SetModel(Model* model){ object3d_.SetModel(model); }
	void SetCamera(Camera* camera){ object3d_.SetCamera(camera); }

	void SetEnableLighting(const int32_t& enableLighting, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetEnableLighting(enableLighting, materialIndex); }
	void SetColor(const Vector4& color, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetColor(color, materialIndex); }
	void SetUVTransform(const EulerTransform& uvTransform, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetUVTransform(uvTransform, materialIndex); }
	void SetShininess(const float& shiniess, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetShininess(shiniess, materialIndex); }
	void SetEnvironmentCoefficient(const float& environmentCoefficient, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetEnvironmentCoefficient(environmentCoefficient, materialIndex); }
	void SetAlphaReference(const float alphaReference, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetAlphaReference(alphaReference, materialIndex); }
	void SetTexture(const std::string& texturePath, const uint32_t* materialIndex = nullptr) 
		{ object3d_.SetTexture(texturePath, materialIndex); }

	void SetParent(WorldTransform* worldTransform){ object3d_.SetParent(worldTransform); }

	// ゲッター
	const std::string& GetPsoName() const { return object3d_.GetPsoName(); }
	const PSOManager::BlendMode& GetBlendMode() const { return object3d_.GetBlendMode(); }
	const PSOManager::FillMode& GetFillMode() const { return object3d_.GetFillMode(); }

	const Vector3& GetScale() const { return object3d_.GetScale(); }
	const Vector3& GetRotate() const { return object3d_.GetRotate(); }
	const Vector3& GetTranslate() const { return object3d_.GetTranslate(); }
	const EulerTransform& GetTransform() const { return object3d_.GetTransform(); }
	WorldTransform* GetWorldTransform() { return object3d_.GetWorldTransform(); }

	const int32_t& GetEnableLighting() const { return object3d_.GetEnableLighting(); }
	const Vector4& GetColor() const { return object3d_.GetColor(); }

	Model* GetModel() const { return object3d_.GetModel(); }
	MaterialInstance* GetMaterial(const size_t& index) const { return object3d_.GetMaterial(index); }

    Object3d* GetObject3d() { return &object3d_; }

    const AnimationPlayer& GetAnimationPlayer() const { return animationPlayer_; }

    const SkeletonPose& GetSkeletonPose() const { return skeletonPose_; }

private:
    //void UpdateAnimation();
    void UpdateSkinningPalette();
    void DispatchSkinning();

private:
    std::shared_ptr<const AnimatedModelAsset> asset_;

    Object3d object3d_;

    AnimationPlayer animationPlayer_;
    SkeletonPose skeletonPose_;

    std::vector<std::unique_ptr<MeshSkin>> meshSkins_;

    std::string computePsoName_ = "Skinning";
};

