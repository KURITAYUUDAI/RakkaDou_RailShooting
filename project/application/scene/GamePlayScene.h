#pragma once
#include "BaseScene.h"
#include "Framework.h"

#include "LightManager.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "collision/CollisionManager.h"


#include "DefaultCameraController.h"
#include "FollowCameraController.h"
#include "RailCameraController.h"
#include "RailRoot.h"
#include "SkyBox.h"
#include "Terrain.h"
#include "Player.h"
#include "Enemy/EnemyManager.h"

#include "EventBus.h"
#include "GamePlaySceneUI.h"

#include "PostEffectController.h"

class GamePlayScene : public BaseScene
{
public:

	~GamePlayScene() override = default;

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

	// 衝突マネージャー
	CollisionManager* collisionManager_ = CollisionManager::GetInstance();

	// 敵マネージャー
	EnemyManager* enemyManager_ = EnemyManager::GetInstance();


private:

	bool isDrawSprite_ = false;

	bool isDrawObject3d_ = true;

	// カメラ
	std::unique_ptr<Camera> camera_ = nullptr;

	/*std::unique_ptr<DebugCamera> debugCamera_;*/
	bool isDebugCamera_ = false;
	int cameraControllerIndex_ = 1;

	std::unique_ptr<DefaultCameraController> defaultCameraController_ = nullptr;

	std::unique_ptr<RailCameraController> railCameraController_ = nullptr;
	std::unique_ptr<FollowCameraController> followCameraController_ = nullptr;
	std::unique_ptr<RailRoot> railRoot_ = nullptr;
	

	Vector2 mousePosition_ = { 0.0f, 0.0f };

	SkyBox* skyBox_ = nullptr;

	std::unique_ptr<Terrain> terrain_ = nullptr;
	std::unique_ptr<Object3d> inFieldObject_ = nullptr;
	std::unique_ptr<Object3d> outFieldObject_ = nullptr;

	std::unique_ptr<Player> player_ = nullptr;

	std::unique_ptr<Enemy> enemy_ = nullptr;
	std::vector<EnemyID> enemyIDs_;

	std::unique_ptr<ParticleEmitter> slashEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;

	std::unique_ptr<EventBus> eventBus_;

	std::unique_ptr<GamePlaySceneUI> sceneUI_;

	std::unique_ptr<Sprite> controlSprite_;

	PostEffectHandle bloomHandle_;
	PostEffectHandle glayscaleHandle_;
};

