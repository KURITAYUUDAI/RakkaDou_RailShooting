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
#include "PauseMenu.h"

#include "PostEffectController.h"

#include <array>

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
	enum class StageEntryState
	{
		WaitingForFadeIn,
		BlackHold,
		Approaching,
		OpeningDoor,
		Diving,
		WhiteOut,
		Skydiving,
		CaveEntry,
		Connecting,
		Complete,
	};

	void InitializeStageEntry();
	void UpdateStageEntry(const float& deltaTime);
	void BeginStageEntrySkydiving();
	void BeginStageEntryRailConnection();
	void UpdateStageEntrySkydivingCamera();
	void UpdateStageEntryCaveFog();
	void UpdateCaveFogEndPlane();
	void CompleteStageEntry();
	void UpdateStageEntryObjects();
	void DrawStageEntryDarkSpace();
	void DrawStageEntryField();
	void StartStageEntryBlur();
	void UpdateStageEntryBlur(float progress);
	void StopStageEntryBlur();
	bool IsStageEntryComplete() const
	{
		return stageEntryState_ == StageEntryState::Complete;
	}

	void InitializeEnemyStage();
	void UpdateEnemyStage(const float& deltaTime);
	void SpawnSmallEnemy(const SmallEnemySpawnConfig& spawnConfig);
	void StartBossPhase();
	void ConfigureEnemy(EnemyID id, bool isBoss);
	void DrawEnemyStageDebugUI();

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

	StageEntryState stageEntryState_ = StageEntryState::WaitingForFadeIn;
	float stageEntryTimer_ = 0.0f;
	bool isStageEntryFieldVisible_ = false;
	bool isStageEntryRailActive_ = false;
	bool isEnemyStageInitialized_ = false;
	float stageEntryDoorProgress_ = 0.0f;

	std::unique_ptr<Object3d> stageEntryBackdrop_ = nullptr;
	std::unique_ptr<Object3d> stageEntryLight_ = nullptr;
	std::unique_ptr<Object3d> stageEntryLeftDoor_ = nullptr;
	std::unique_ptr<Object3d> stageEntryRightDoor_ = nullptr;
	std::array<std::unique_ptr<Object3d>, 3> stageEntryDoorFrames_;
	std::unique_ptr<Object3d> stageEntryFieldEntrance_ = nullptr;
	std::unique_ptr<Object3d> stageEntryFieldGround_ = nullptr;
	std::unique_ptr<Object3d> caveFogEndPlane_ = nullptr;
	std::unique_ptr<Sprite> stageEntryWhiteSprite_ = nullptr;
	Vector3 stageEntryFieldEntranceScale_ = { 1.0f, 1.0f, 1.0f };
	Vector3 stageEntryFieldEntrancePosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 stageEntryFieldGroundScale_ = { 1.0f, 1.0f, 1.0f };
	Vector3 stageEntryFieldGroundPosition_ = { 0.0f, 0.0f, 0.0f };
	float stageEntrySkydivingStartHeight_ = 500.0f;
	float stageEntrySkydivingInitialSpeed_ = 20.0f;
	float stageEntrySkydivingAcceleration_ = 80.0f;
	float stageEntrySkydivingCurrentSpeed_ = 0.0f;
	float stageEntryCameraForwardHoldDuration_ = 0.25f;
	float stageEntryCameraTurnDuration_ = 0.75f;
	float stageEntryCameraTurnTimer_ = 0.0f;
	float stageEntryCaveEntryStartHeight_ = 50.0f;
	float stageEntryRailConnectHeight_ = -30.0f;
	float stageEntryCameraHeightOffset_ = 15.0f;
	float stageEntryCaveFogBaseStart_ = 20.0f;
	float stageEntryCaveFogBaseEnd_ = 260.0f;
	float caveFogEndPlaneScale_ = 50.0f;
	float caveFogEndPlaneOffset_ = 0.0f;

	PostEffectHandle stageEntryBlurHandle_ = kInvalidPostEffectHandle;

	static constexpr float kStageEntryBlackHoldDuration_ = 0.25f;
	static constexpr float kStageEntryApproachDuration_ = 0.90f;
	static constexpr float kStageEntryDoorOpenDuration_ = 0.65f;
	static constexpr float kStageEntryDiveDuration_ = 0.55f;
	static constexpr float kStageEntryWhiteOutDuration_ = 0.20f;
	static constexpr float kStageEntryWhiteFadeDuration_ = 0.45f;
	static constexpr float kStageEntryConnectDuration_ = 0.35f;
	static constexpr float kStageEntryNormalRailSpeed_ = 100.0f;
	

	Vector2 mousePosition_ = { 0.0f, 0.0f };

	SkyBox* skyBox_ = nullptr;

	std::unique_ptr<Terrain> terrain_ = nullptr;

	std::unique_ptr<Player> player_ = nullptr;

	EnemyStageConfig enemyStageConfig_;
	enum class EnemyStagePhase
	{
		SmallEnemies,
		Boss,
	};
	EnemyStagePhase enemyStagePhase_ = EnemyStagePhase::SmallEnemies;
	size_t nextSmallEnemyIndex_ = 0;
	EnemyID bossEnemyID_ = 0;

	std::unique_ptr<ParticleEmitter> slashEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;

	std::unique_ptr<EventBus> eventBus_;

	std::unique_ptr<GamePlaySceneUI> sceneUI_;

	std::unique_ptr<PauseMenu> pauseMenu_;
	bool isPaused_ = false;

	PostEffectHandle bloomHandle_;
	PostEffectHandle glayscaleHandle_;
};

