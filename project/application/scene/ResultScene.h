#pragma once
#include "BaseScene.h"
#include "Framework.h"
#include "CameraManager.h"
#include "DefaultCameraController.h"
#include "SkyBox.h"

class ResultScene : public BaseScene
{
public: 
	
	~ResultScene() override = default;

public:

	void Initialize() override;

	void Finalize() override;

	void Update(const float& deltaTime) override;

	void FinishFadeIn() override;

	void Draw() override;

private:

	std::vector<std::unique_ptr<Sprite>> sprites_;

	std::vector<std::unique_ptr<Object3d>> object3ds_;

	std::list<std::unique_ptr<ParticleEmitter>> emitters_;

private:

	// テクスチャマネージャー
	TextureManager* textureManager_ = TextureManager::GetInstance();

	// パーティクルマネージャー
	ParticleManager* particleManager_ = ParticleManager::GetInstance();

	// インプットマネージャー
	InputManager* inputManager_ = InputManager::GetInstance();

	// スプライトマネージャー
	SpriteManager* spriteManager_ = SpriteManager::GetInstance();

	// 3Dオブジェクトマネージャー
	Object3dManager* object3dManager_ = Object3dManager::GetInstance();

	// カメラマネージャー
	CameraManager* cameraManager_ = CameraManager::GetInstance();

private:

	bool isDrawSprite_ = true;

	bool isDrawObject3d_ = true;
	float boxAnimationTime_ = 0.0f;

	// カメラ
	std::unique_ptr<Camera> camera_ = nullptr;
	// 背景
	std::unique_ptr<SkyBox> skyBox_ = nullptr;
	bool isDebugCamera_ = false;
	std::unique_ptr<DefaultCameraController> defaultCameraController_ = nullptr;

	Vector2 mousePosition_ = { 0.0f, 0.0f };

	std::unique_ptr<Sprite> enterSprite_;
};

