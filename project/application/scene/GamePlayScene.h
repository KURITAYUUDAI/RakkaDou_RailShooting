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
#include "application/object/cave/CaveRailPath.h"
#include "Player.h"
#include "Enemy/EnemyManager.h"
#include "Enemy/EnemyStageConfig.h"

#include "EventBus.h"
#include "GamePlaySceneUI.h"
#include "PauseMenu.h"

#include "PostEffectController.h"
#include "application/command/input/InputMapping.h"
#include "gameSequence/GamePlaySequence.h"

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

	void Draw(CanvasId canvasId) override;

private:
	enum class ScenePhase
	{
		Entry,
		Play,
		Clear,
		GameOver,
	};

	enum class GameplayState
	{
		Inactive,
		Skydiving,
		CaveEntering,
		Playing,
	};

	enum class StageClearState
	{
		Inactive,
		RepositioningPlayer,
		DisplayingText,
		Diving,
		WaitingForTransition,
		Transitioning,
	};

	void InitializeGameplayPresentation();
	void InitializeEntrySequence();
	void UpdateEntrySequence(float deltaTime);
	void FinishEntrySequence();
	void UpdateGameplayState(float deltaTime);
	void BeginSkydivingGameplay();
	void BeginCaveEntering();
	void BeginNormalGameplay();
	void ActivateGameplay();
	void UpdateSkydivingSpeed(float deltaTime);
	void UpdateCaveFog();
	void UpdateGameplayWindLines(float deltaTime);
	void UpdateCaveFogEndPlane();
	void UpdateStageTitle(float deltaTime);
	void DrawStageTitle();
	void StartStageClearSequence();
	void UpdateStageClearSequence(float deltaTime);
	void DrawStageClearText();
	void DrawGameplayEntryField();
	void StartStageEntryBlur();
	void UpdateStageEntryBlur(float progress, float maxWidth = 0.02f);
	void StopStageEntryBlur();
	bool IsStageEntryComplete() const
	{
		return scenePhase_ != ScenePhase::Entry;
	}
	bool IsStageClearSequenceActive() const
	{
		return stageClearState_ != StageClearState::Inactive;
	}

	void InitializeEnemyStage();
	void UpdateGameplayDebugUI();
	void UpdateGameplaySystems(const float& deltaTime);
	GameUIFrameSnapshot BuildGameUIFrameSnapshot() const;
	void StartGameOverSequence();
	void UpdateEnemyStage(const float& deltaTime);
	void SpawnSmallEnemy(
		const SmallEnemySpawnConfig& spawnConfig, size_t spawnIndex);
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
	InputMapping* inputMapping_ = InputMapping::GetInstance();

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
	std::unique_ptr<CaveRailPath> caveRailPath_ = nullptr;

	ScenePhase scenePhase_ = ScenePhase::Entry;
	std::unique_ptr<GamePlaySequence> activeSequence_ = nullptr;
	GameplayState gameplayState_ = GameplayState::Inactive;
	float gameplayStateTimer_ = 0.0f;
	bool isEnemyStageInitialized_ = false;
	std::unique_ptr<Object3d> gameplayEntryFieldEntrance_ = nullptr;
	std::unique_ptr<Object3d> gameplayEntryFieldGround_ = nullptr;
	std::unique_ptr<Object3d> caveFogEndPlane_ = nullptr;
	std::unique_ptr<Text> stageTitleText_ = nullptr;
	std::unique_ptr<Text> stageClearText_ = nullptr;
	Vector3 gameplayEntryFieldEntranceScale_ = { 1.0f, 1.0f, 1.0f };
	Vector3 gameplayEntryFieldEntrancePosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 gameplayEntryFieldGroundScale_ = { 1.0f, 1.0f, 1.0f };
	Vector3 gameplayEntryFieldGroundPosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 stageEntryDoorPosition_ = { 0.0f, 3.2f, 8.0f };
	Vector3 stageEntryPlayerStartPosition_ = { 0.0f, -1.7f, -7.0f };
	Vector3 stageEntryPlayerDoorPosition_ = { 0.0f, 0.0f, 1.5f };
	Vector3 stageEntryPlayerDiveEndPosition_ = { 0.0f, 0.0f, 10.5f };
	Vector3 gameplayPlayerRailPosition_ = { 0.0f, -1.7f, -7.5f };
	float skydivingStartHeight_ = 500.0f;
	float skydivingInitialSpeed_ = 20.0f;
	float skydivingAcceleration_ = 80.0f;
	float skydivingCurrentSpeed_ = 0.0f;
	float lookDownCameraForwardHoldDuration_ = 0.25f;
	float lookDownCameraTurnDuration_ = 0.75f;
	float caveFogBaseStart_ = 20.0f;
	float caveFogBaseEnd_ = 500.0f;
	float caveFogEndPlaneScale_ = 50.0f;
	float caveFogEndPlaneOffset_ = 0.0f;
	float gameplayWindLineEmitAccumulator_ = 0.0f;
	float gameplayWindLineMinRadius_ = 6.0f;
	float gameplayWindLineMaxRadius_ = 32.0f;
	float gameplayWindLineMinDepth_ = 30.0f;
	float gameplayWindLineMaxDepth_ = 150.0f;
	float gameplayWindLineMinLength_ = 3.0f;
	float gameplayWindLineMaxLength_ = 22.0f;
	float gameplayWindLineMaxEmitRate_ = 48.0f;
	float gameplayWindLineAlpha_ = 0.28f;
	float stageTitleTimer_ = 0.0f;
	bool isStageTitleTimerActive_ = false;
	static constexpr const char* kGameplayWindLineGroup_ =
		"stage_entry_wind_line";
	static constexpr float kStageTitleDelay_ = 1.0f;
	static constexpr float kStageTitleDuration_ = 3.0f;

	PostEffectHandle stageEntryBlurHandle_ = kInvalidPostEffectHandle;

	static constexpr float kStageEntryBlackHoldDuration_ = 0.25f;
	static constexpr float kStageEntryApproachDuration_ = 0.90f;
	static constexpr float kStageEntryDoorOpenDuration_ = 0.65f;
	static constexpr float kStageEntryDiveDuration_ = 0.55f;
	static constexpr float kStageEntryWhiteOutDuration_ = 0.20f;
	static constexpr float kStageEntryWhiteFadeDuration_ = 0.45f;
	static constexpr float kGameplayCaveEnterDuration_ = 0.35f;
	static constexpr float kNormalRailSpeed_ = 300.0f;
	

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
	float enemyStageSpawnDelay_ = 4.0f;
	size_t nextSmallEnemyIndex_ = 0;
	EnemyID bossEnemyID_ = 0;
	std::vector<EnemyManager::PoolHandle> smallEnemyPoolHandles_;
	EnemyManager::PoolHandle bossEnemyPoolHandle_ =
		EnemyManager::kInvalidPoolHandle;

	std::unique_ptr<ParticleEmitter> slashEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> stageClearShockwaveEmitter_ = nullptr;

	std::unique_ptr<EventBus> eventBus_;
	EventSubscriber eventSubscriber_;

	std::unique_ptr<GamePlaySceneUI> sceneUI_;

	std::unique_ptr<PauseMenu> pauseMenu_;
	bool isPaused_ = false;
	StageClearState stageClearState_ = StageClearState::Inactive;
	float stageClearTimer_ = 0.0f;
	Vector3 stageClearPlayerStartPosition_{};
	Vector3 stageClearDiveStartPosition_{};
	static constexpr float kStageClearRepositionDuration_ = 1.0f;
	static constexpr float kStageClearTextDuration_ = 2.0f;
	static constexpr float kStageClearDiveDuration_ = 1.0f;
	static constexpr float kStageClearDiveDistance_ = 500.0f;
	static constexpr float kStageClearTransitionDelay_ = 0.5f;
	static constexpr float kStageClearBlurMaxWidth_ = 0.015f;
	PostEffectHandle bloomHandle_;
	PostEffectHandle glayscaleHandle_;
};

