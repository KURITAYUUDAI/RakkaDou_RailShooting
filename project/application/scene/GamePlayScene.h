#pragma once
#include "BaseScene.h"
#include "Framework.h"

#include "LightManager.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "collision/CollisionManager.h"


#include "DefaultCameraController.h"
#include "RailCameraController.h"
#include "SkyBox.h"
#include "Terrain.h"
#include "Player.h"
#include "Enemy/EnemyManager.h"
#include "Enemy/EnemyStageConfig.h"

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

	void InitializeEnemyStage();
	void UpdateEnemyStage(const float& deltaTime);
	void SpawnSmallEnemy(const SmallEnemySpawnConfig& spawnConfig);
	void StartBossPhase();
	void ConfigureEnemy(EnemyID id, bool isBoss);

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

	std::unique_ptr<DefaultCameraController> defaultCameraController_ = nullptr;

	std::unique_ptr<RailCameraController> railCameraController_ = nullptr;
	

	Vector2 mousePosition_ = { 0.0f, 0.0f };

	SkyBox* skyBox_ = nullptr;

	std::unique_ptr<Terrain> terrain_ = nullptr;

	std::unique_ptr<Object3d> inField_ = nullptr;
	std::unique_ptr<Object3d> outField_ = nullptr;

	std::unique_ptr<Player> player_ = nullptr;

	EnemyStageConfig enemyStageConfig_;
	enum class EnemyStagePhase
	{
		SmallEnemies,
		Boss,
	};
	EnemyStagePhase enemyStagePhase_ = EnemyStagePhase::SmallEnemies;
	float enemyStageTimer_ = 0.0f;
	size_t nextSmallEnemyIndex_ = 0;
	EnemyID bossEnemyID_ = 0;

	std::unique_ptr<ParticleEmitter> slashEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;

	std::unique_ptr<EventBus> eventBus_;

	std::unique_ptr<GamePlaySceneUI> sceneUI_;

	std::unique_ptr<Sprite> controlSprite_;

	PostEffectHandle bloomHandle_;
	PostEffectHandle glayscaleHandle_;
};

