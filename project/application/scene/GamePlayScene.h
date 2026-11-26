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
#include "application/object/cave/CaveGameplayEnvironment.h"
#include "application/object/cave/CaveRailPath.h"
#include "application/object/cave/StageProgressLine.h"
#include "Player.h"
#include "Enemy/EnemyManager.h"
#include "Enemy/EnemyStageDirector.h"

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

	void InitializeGameplayPresentation();
	void InitializeEntrySequence();
	void UpdateEntrySequence(float deltaTime);
	void FinishEntrySequence();
	void BeginSkydivingGameplay();
	void ActivateGameplay();
	void UpdateStageTitle(float deltaTime);
	void DrawStageTitle();
	void StartStageClearSequence();
	bool IsStageEntryComplete() const
	{
		return scenePhase_ != ScenePhase::Entry;
	}
	bool IsStageClearSequenceActive() const
	{
		return scenePhase_ == ScenePhase::Clear;
	}

	void UpdateGameplayDebugUI();
	void UpdateGameplaySystems(const float& deltaTime);
	GameUIFrameSnapshot BuildGameUIFrameSnapshot() const;
	void StartGameOverSequence();

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
	std::unique_ptr<StageProgressLine> stageProgressLine_ = nullptr;
	std::unique_ptr<Text> stageTitleText_ = nullptr;
	Vector3 stageEntryDoorPosition_ = { 0.0f, 3.2f, 8.0f };
	Vector3 stageEntryPlayerStartPosition_ = { 0.0f, -1.7f, -7.0f };
	Vector3 stageEntryPlayerDoorPosition_ = { 0.0f, 0.0f, 1.5f };
	Vector3 stageEntryPlayerDiveEndPosition_ = { 0.0f, 0.0f, 10.5f };
	Vector3 gameplayPlayerRailPosition_ = { 0.0f, -1.7f, -7.5f };
	float lookDownCameraForwardHoldDuration_ = 0.25f;
	float lookDownCameraTurnDuration_ = 0.75f;
	float stageTitleTimer_ = 0.0f;
	bool isStageTitleTimerActive_ = false;
	static constexpr float kStageTitleDelay_ = 1.0f;
	static constexpr float kStageTitleDuration_ = 3.0f;

	static constexpr float kStageEntryBlackHoldDuration_ = 0.25f;
	static constexpr float kStageEntryApproachDuration_ = 0.90f;
	static constexpr float kStageEntryDoorOpenDuration_ = 0.65f;
	static constexpr float kStageEntryDiveDuration_ = 0.55f;
	static constexpr float kStageEntryWhiteOutDuration_ = 0.20f;
	static constexpr float kStageEntryWhiteFadeDuration_ = 0.45f;
	Vector2 mousePosition_ = { 0.0f, 0.0f };

	std::unique_ptr<CaveGameplayEnvironment> caveEnvironment_ = nullptr;

	std::unique_ptr<Player> player_ = nullptr;

	std::unique_ptr<EnemyStageDirector> enemyStageDirector_ = nullptr;

	std::unique_ptr<ParticleEmitter> slashEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> ringEmitter_ = nullptr;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_ = nullptr;
	std::unique_ptr<ParticleEmitter> stageClearShockwaveEmitter_ = nullptr;

	std::unique_ptr<EventBus> eventBus_;
	EventSubscriber eventSubscriber_;

	std::unique_ptr<GamePlaySceneUI> sceneUI_;

	std::unique_ptr<PauseMenu> pauseMenu_;
	bool isPaused_ = false;
	PostEffectHandle bloomHandle_;
	PostEffectHandle glayscaleHandle_;
};

