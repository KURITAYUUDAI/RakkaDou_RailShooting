#pragma once
#include "BaseScene.h"
#include "Framework.h"

#include "Text.h"
#include "CameraManager.h"
#include "DefaultCameraController.h"

#include "LightManager.h"
#include "DebugDrawManager.h"
#include "PrimitiveManager.h"
#include "SkyBox.h"
#include "Terrain.h"

class TitleScene : public BaseScene
{
public:

	~TitleScene() override = default;

public:

	void Initialize() override;

	void Finalize() override;

	void Update(const float& deltaTime) override;

	void FinishFadeIn() override;

	void Draw() override;

private:
	void UpdateOrbitCamera(float deltaTime);

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

	// モデルマネージャー
	ModelManager* modelManager_ = ModelManager::GetInstance();

	// 3Dオブジェクトマネージャー
	Object3dManager* object3dManager_ = Object3dManager::GetInstance();

	// サウンドマネージャー
	SoundManager* soundManager_ = SoundManager::GetInstance();

	// ライトマネージャー
	LightManager* lightManager_ = LightManager::GetInstance();

	// カメラマネージャー
	CameraManager* cameraManager_ = CameraManager::GetInstance();

	// デバッグ描画マネージャー
	DebugDrawManager* debugManager_ = DebugDrawManager::GetInstance();

	// プリミティブマネージャー
	PrimitiveManager* primitiveManager_ = PrimitiveManager::GetInstance();

private:

	bool isDrawSprite_ = true;

	bool isDrawObject3d_ = false;

	// カメラ
	std::unique_ptr<Camera> camera_ = nullptr;

	/*std::unique_ptr<DebugCamera> debugCamera_;*/
	bool isDebugCamera_ = false;

	std::unique_ptr<DefaultCameraController> defaultCameraController_ = nullptr;
	std::unique_ptr<SkyBox> skyBox_ = nullptr;
	std::unique_ptr<Terrain> terrain_ = nullptr;

	Vector2 mousePosition_ = { 0.0f, 0.0f };

	std::unique_ptr<Sprite> enterSprite_;

	std::unique_ptr<Text> titleText_;

	std::unique_ptr<ParticleEmitter> ringEmitter_;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_;

	std::unique_ptr<Object3d> playerObject_;
	std::unique_ptr<Object3d> rightHandObject_;
	std::unique_ptr<Object3d> inFieldObject_;
	std::unique_ptr<Object3d> outFieldObject_;
	float cameraOrbitAngle_ = 0.0f;
	static constexpr float kCameraOrbitSpeed_ = 0.25f;
	static constexpr float kCameraOrbitRadius_ = 10.0f;
	static constexpr float kCameraOrbitHeight_ = 2.0f;
	static constexpr Vector3 kCameraLookTarget_ = { 15.0f, 0.0f, 15.0f };

};

