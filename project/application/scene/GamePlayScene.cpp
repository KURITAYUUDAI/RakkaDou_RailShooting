#define NOMINMAX
#include "GamePlayScene.h"
#include "SceneManager.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "PrimitiveManager.h"
#include "PostEffectManager.h"
#include "GaussianBlur.h"
#include "Dissolve.h"
#include "Random.h"
#include "Bloom.h"
#include "Grayscale.h"
#include "BulletManager.h"
#include "SplineCurve.h"
#include "HPGageUI.h"
#include "Darken.h"
#include "DistanceFogManager.h"
#include "freetype/FreeTypeManager.h"
#include "FontManager.h"
#include "TextManager.h"
#include "Enemy/EnemyEvent.h"
#include "gameSequence/GameClearSequence.h"
#include "gameSequence/GameOverSequence.h"
#include "gameSequence/StageEntrySequence.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace
{
	constexpr float kGameplayBattleSpaceCenterZ = 22.5f;
	constexpr float kBossDefeatedFlashIntensity = 0.7f;
	constexpr float kBossDefeatedFlashHoldDuration = 0.1f;
	constexpr float kBossDefeatedFlashFadeDuration = 0.4f;
	constexpr float kBossDefeatedFlashDuration =
		kBossDefeatedFlashHoldDuration + kBossDefeatedFlashFadeDuration;
	constexpr int32_t kBossDefeatedFlashPriority = 1000;

	Vector3 CalculateGameplayCameraForwardOffset(const Vector3& playerLocalPosition)
	{
		return {
			-playerLocalPosition.x,
			-playerLocalPosition.y,
			-(kGameplayBattleSpaceCenterZ + playerLocalPosition.z)
		};
	}

	float SmoothStep01(float value)
	{
		const float t = std::clamp(value, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	float LerpFloat(float start, float end, float t)
	{
		return start + (end - start) * t;
	}

}

void GamePlayScene::Initialize()
{
	scenePhase_ = ScenePhase::Entry;
	activeSequence_.reset();
	inputMapping_->SetContext(InputContext::Gameplay);
	const StageEntryConfig defaultEntryConfig{};

	camera_ = std::make_unique<Camera>();
	camera_->Initialize();

	// Textureを読んで転送する
	textureManager_->LoadTexture("uvChecker.png");
	textureManager_->LoadTexture("monsterBall.png");
	textureManager_->LoadTexture("rostock_laage_airport_4k.dds");

	int textureIndex = 0;
	const char* textureOptions[] = { "Checker", "monsterBall" };

	// シーン初期化始め
	/*camera_->SetRotate({ pi / 3.0f, pi, 0.0f });
	camera_->SetTranslate({ 0.0f, 9.0f, 5.0f });*/

	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate(
		defaultEntryConfig.playerStartPosition +
		CalculateGameplayCameraForwardOffset(gameplayPlayerRailPosition_));
	camera_->SetFarClip(600.0f);

	/*debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();
	debugCamera_->SetRotate(camera_->GetRotate());
	debugCamera_->SetTranslate(camera_->GetTranslate());*/

	cameraManager_->Initialize();
	cameraManager_->AddCamera("Default", camera_.get());
	/*cameraManager_->AddCamera("Debug", debugCamera_.get());*/
	cameraManager_->SetActiveCamera("Default");

	defaultCameraController_ = std::make_unique<DefaultCameraController>();
	defaultCameraController_->Initialize();
	defaultCameraController_->GetWorldTransform()->SetTranslate(
		defaultEntryConfig.playerStartPosition +
		CalculateGameplayCameraForwardOffset(gameplayPlayerRailPosition_));
	defaultCameraController_->GetWorldTransform()->SetRotate({ 0.0f, 0.0f, 0.0f });

	caveRailPath_ = std::make_unique<CaveRailPath>();
	caveRailPath_->Initialize();

	railCameraController_ = std::make_unique<RailCameraController>();
	railCameraController_->SetCaveRailPath(caveRailPath_.get());
	railCameraController_->Initialize(RailCameraController::Mode::VerticalDescent);
	railCameraController_->SetMovementEnabled(false);
	railCameraController_->SetMoveSpeed(20.0f);

	cameraManager_->AddCameraController("Default", defaultCameraController_.get());
	cameraManager_->AddCameraController("Rail", railCameraController_.get());
	cameraManager_->SetActiveCameraController("Default");

	lightManager_->Initialize();
	lightManager_->SetDirectionalLightColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	lightManager_->SetDirectionalLightDirection({ 0.0f, -1.0f, 0.0f });
	lightManager_->SetDirectionalLightIntensity(1.0f);

	/*lightManager_->SetNumPointLights(1);
	lightManager_->SetPointLightColor(0, { 1.0f, 0.0f, 0.0f, 1.0f });
	lightManager_->SetPointLightPosition(0, { 0.0f, 1.0f, 0.0f });
	lightManager_->SetPointLightIntensity(0, 1.0f);
	lightManager_->SetPointLightRadius(0, 5.0f);
	lightManager_->SetPointLightDecay(0, 1.0f);

	lightManager_->SetNumSpotLights(1);
	lightManager_->SetSpotLightColor(0, { 0.0f, 1.0f, 0.0f, 1.0f });
	lightManager_->SetSpotLightPosition(0, { 2.0f, 1.25f, 0.0f });
	lightManager_->SetSpotLightIntensity(0, 4.0f);
	lightManager_->SetSpotLightDirection(0, { -1.0f, -1.0f, 0.0f });
	lightManager_->SetSpotLightDistance(0, 7.0f);
	lightManager_->SetSpotLightDecay(0, 1.0f);
	lightManager_->SetSpotLightCosAngle(0, std::cosf(pi / 3.0f));
	lightManager_->SetSpotLightCosFalloff(0, std::cosf(pi / 4.0f));*/

	auto* distanceFog = DistanceFogManager::GetInstance();
	distanceFog->SetStartDistance(20.0f);
	distanceFog->SetEndDistance(500.0f);
	distanceFog->SetColor({ 0.005f, 0.008f, 0.015f, 1.0f });
	distanceFog->SetEnabled(true);

	for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<Sprite> newSprite = std::make_unique<Sprite>();
		newSprite->Initialize("uvChecker.png");
		newSprite->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		sprites_.push_back(std::move(newSprite));
	}

	sprites_[0]->SetTexture("monsterBall.png");
	sprites_[0]->SetPosition(Vector2{ 100.0f, 100.0f });
	sprites_[0]->AdjustTextureSize();

	modelManager_->LoadModel("", "plane.obj");
	modelManager_->LoadModel("", "axis.obj");
	modelManager_->LoadModel("", "sphere.obj");
	modelManager_->LoadModel("", "multiMaterial.obj");
	for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<Object3d> newObject3d = std::make_unique<Object3d>();
		newObject3d->Initialize();
		newObject3d->SetModel("plane.obj");
		object3ds_.push_back(std::move(newObject3d));
	}

	object3ds_[0]->SetModel("sphere.obj");

	/*particleManager_->CreateParticleGroup("circle", "circle.png");
	particleManager_->SetModel("circle", "plane.obj");
	particleManager_->SetIsMoveAccelerationField("circle", true);*/
	particleManager_->CreateParticleGroup("slash", "circle2.png");
	particleManager_->SetModel("slash", "plane.obj");
	particleManager_->SetIsMoveAccelerationField("slash", false);

	PrimitiveManager::GetInstance()->Initialize();
	PrimitiveManager::RingConfig ringConfig;
	ringConfig.segments = 32;
	ringConfig.innerRadius = 0.0f;
	ringConfig.outerRadius = 1.5f;
	ringConfig.innerColor = {0.0f, 0.0f, 1.0f, 1.0f};
	ringConfig.outerColor = {0.0f, 0.5f, 0.0f, 1.0f};
	ringConfig.uvScaleU = 2.0f;
	ringConfig.uvScaleV = 0.1f;
	ringConfig.startAngle = 1.0f / 5.0f * pi;
	ringConfig.endAngle = 9.0f / 5.0f * pi;
	ringConfig.radiusPoints = 
	{
		{ 0.0f, 0.5f, 0.5f },   // 開始：inner=0.2, outer=1.0
		{ 0.5f, 0.5f, 1.0f },   // 中間：inner=0.1, outer=0.6
		{ 1.0f, 0.5f, 0.5f },   // 終了：inner=0.2, outer=0.2
	};
	ringConfig.alphaFade.startFadeRange = 0.15f;
	ringConfig.alphaFade.endFadeRange = 0.15f;
	PrimitiveManager::GetInstance()->CreateRing("ring_primitive", ringConfig);
	
	particleManager_->CreateParticleGroup("ring", "gradationLine.png");
	particleManager_->SetModel("ring", "ring_primitive");
	particleManager_->SetIsMoveAccelerationField("ring", false);
	particleManager_->SetIsBillboard("ring", false);

	PrimitiveManager::CylinderConfig cylinderConfig;
	cylinderConfig.segments = 32;
	cylinderConfig.stacks = 32;
	cylinderConfig.height = 3.0f;
	cylinderConfig.topRadiusX = 1.0f;
	cylinderConfig.topRadiusZ = 1.0f;
	cylinderConfig.bottomRadiusX = 1.0f;
	cylinderConfig.bottomRadiusZ = 1.0f;
	cylinderConfig.radiusPoints =
	{
		{ 0.0f, 1.0f, 1.0f, false },  // ここから折れ線
		{ 0.3f, 2.0f, 2.0f, true  },  // ここからなめらか
		{ 0.7f, 2.0f, 2.0f, false },  // ここから折れ線
		{ 1.0f, 1.0f, 1.0f },
	};
	cylinderConfig.topColor = { 0.0f, 0.0f, 1.0f, 1.0f };
	cylinderConfig.bottomColor = { 0.0f, 0.5f, 0.0f, 1.0f };
	cylinderConfig.uvScaleU = 2.0f;
	cylinderConfig.uvScaleV = 1.0f;
	cylinderConfig.startAngle = 0.0f;
	cylinderConfig.endAngle = 2.0f * pi;
	cylinderConfig.alphaFade.startFadeRange = 0.15f;
	cylinderConfig.alphaFade.endFadeRange = 0.15f;
	PrimitiveManager::GetInstance()->CreateCylinder("cylinder_primitive", cylinderConfig);
	particleManager_->CreateParticleGroup("cylinder", "uvChecker.png");
	particleManager_->SetModel("cylinder", "cylinder_primitive");
	particleManager_->SetIsMoveAccelerationField("cylinder", false);
	particleManager_->SetIsBillboard("cylinder", false);

	AABB  aabb;
	aabb.max = { 1.0f, 1.0f, 1.0f };
	aabb.min = { -1.0f, -1.0f, -1.0f };
	particleManager_->CreateAccelerationField({ 5.0f, 0.0f, 0.0f }, aabb);


	/*for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<ParticleEmitter> emitter = std::make_unique<ParticleEmitter>();
		Transform transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
		emitter->Initialize("circle", transform, 3, 0.2f);
		emitters_.push_back(std::move(emitter));
	}*/

	StageProgressLineConfig progressLineConfig{};
	caveEnvironment_ = std::make_unique<CaveGameplayEnvironment>(
		CaveGameplayEnvironmentDependencies{
			*camera_,
			*cameraManager_,
			*railCameraController_,
			*caveRailPath_,
			*particleManager_,
		},
		CaveGameplayEnvironmentConfig{
			.normalRailSpeed = progressLineConfig.normalRailSpeed,
		});
	caveEnvironment_->Initialize();
	stageProgressLine_ = std::make_unique<StageProgressLine>(
		StageProgressLineDependencies{
			*railCameraController_,
			*caveRailPath_,
			*caveEnvironment_,
		},
		progressLineConfig);
	stageProgressLine_->Reset();

	player_ = std::make_unique<Player>();
	player_->Initialize(Player::Mode::StageEntry);
	player_->SetEnvironmentTextureIndex(
		caveEnvironment_->GetEnvironmentTextureIndex());

	BulletManager::GetInstance()->Initialize();

	slashEmitter_ = std::make_unique<ParticleEmitter>();
	slashEmitter_->Initialize("slash", 
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 3, 0.2f);

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Initialize("ring", 
		{ {1.0f, 2.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 1, 0.2f);

	cylinderEmitter_ = std::make_unique<ParticleEmitter>();
	cylinderEmitter_->Initialize("cylinder",
		{ {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 1, 0.2f);

	bloomHandle_ = SceneManager::GetInstance()->GetPostEffectController()->Emit<Bloom>("Bloom", std::nullopt, nullptr);
	//glayscaleHandle_ = SceneManager::GetInstance()->GetPostEffectController()->Emit<Grayscale>("Grayscale", std::nullopt, nullptr);

	debugManager_->Initialize();

	// シーン初期化終わり

	soundManager_->SoundLoadFile("", "Alarm01.wav");
	soundManager_->SoundLoadFile("", "test.mp3");

	FreeTypeManager::GetInstance()->Initialize();
	TextManager::GetInstance()->Initialize();

	eventBus_ = std::make_unique<EventBus>();
	eventSubscriber_.Initialize(eventBus_.get());
	eventSubscriber_.Subscribe<BossDefeatedSequenceStartedEvent>(
		[this](const BossDefeatedSequenceStartedEvent& event)
		{
			if (!enemyStageDirector_ ||
				!enemyStageDirector_->IsCurrentBoss(event.enemyID))
			{
				return;
			}

			auto* controller = SceneManager::GetInstance()->GetPostEffectController();
			const PostEffectHandle handle = controller->Emit<Darken>(
				BuiltinCanvas::Final,
				"Darken",
				kBossDefeatedFlashDuration,
				[](Darken& flash, float, float elapsedTime)
				{
					float intensity = kBossDefeatedFlashIntensity;
					if (elapsedTime > kBossDefeatedFlashHoldDuration)
					{
						const float fadeProgress = std::clamp(
							(elapsedTime - kBossDefeatedFlashHoldDuration) /
							kBossDefeatedFlashFadeDuration,
							0.0f, 1.0f);
						intensity *= 1.0f - fadeProgress;
					}
					flash.SetDarkColor({ 1.0f, 1.0f, 1.0f, 1.0f });
					flash.SetIntensity(intensity);
				},
				kBossDefeatedFlashPriority);

			// EventBusのDispatchはPostEffectController::Updateより後なので、
			// 発行された最初の描画フレームから白0.7を適用する。
			controller->Edit<Darken>(
				handle,
				[](Darken& flash)
				{
					flash.SetDarkColor({ 1.0f, 1.0f, 1.0f, 1.0f });
					flash.SetIntensity(kBossDefeatedFlashIntensity);
				});
		});
	eventSubscriber_.Subscribe<BossDefeatedSequenceFinishedEvent>(
		[this](const BossDefeatedSequenceFinishedEvent& event)
		{
			if (!enemyStageDirector_ ||
				!enemyStageDirector_->IsCurrentBoss(event.enemyID))
			{
				return;
			}
			StartStageClearSequence();
		});
	sceneUI_ = std::make_unique<GamePlaySceneUI>();
	sceneUI_->Initialize();

	player_->SetEventBus(eventBus_.get());
	player_->EventDispatch();

	enemyManager_->Initialize();
	player_->SetEnemyQueryProvider(enemyManager_);
	enemyManager_->SetRailFollowContext(
		caveRailPath_.get(), railCameraController_->GetCurrentDistance(),
		railCameraController_->GetPathWorldOffset());
	enemyStageDirector_ = std::make_unique<EnemyStageDirector>(
		EnemyStageDependencies{
			*enemyManager_,
			*railCameraController_,
			caveEnvironment_->GetTerrain(),
			caveEnvironment_->GetSkyBox(),
			*eventBus_,
		});
	enemyStageDirector_->Initialize();
	
	eventBus_->Dispatch();

	pauseMenu_ = std::make_unique<PauseMenu>();
	pauseMenu_->Initialize();
	isPaused_ = false;


	stageTitleText_ = std::make_unique<Text>();
	stageTitleText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf", 64, U"STAGE 1");
	stageTitleText_->SetPosition({ 491.0f, 240.0f });
	stageTitleText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	stageTitleText_->Update();

	player_->SetScale({ 1.0f, 1.0f, 1.0f });
	player_->SetRotate({ 0.0f, 0.0f, 0.0f });
	player_->SetTranslate(StageEntryConfig{}.playerStartPosition);
	player_->SetTitleOpacity(0.0f);

	stageProgressLine_->Reset();
	stageTitleTimer_ = 0.0f;
	isStageTitleTimerActive_ = false;

	InitializeEntrySequence();
}

void GamePlayScene::InitializeEntrySequence()
{
	StageEntryDependencies dependencies{
		*player_,
		*cameraManager_,
		*defaultCameraController_,
		*railCameraController_,
		*SceneManager::GetInstance()->GetPostEffectController(),
	};

	StageEntryConfig config{};
	config.gameplayPlayerRailPosition = gameplayPlayerRailPosition_;
	const StageProgressLineConfig& progressLineConfig = stageProgressLine_->GetConfig();
	config.skydivingStartHeight = progressLineConfig.skydivingStartHeight;
	config.skydivingInitialSpeed = progressLineConfig.initialSpeed;
	config.normalRailSpeed = progressLineConfig.normalRailSpeed;

	StartSequence(std::make_unique<StageEntrySequence>(dependencies, config), ScenePhase::Entry);

	if (!SceneManager::GetInstance()->GetSceneTransition().IsRunning())
	{
		activeSequence_->OnFadeInFinished();
	}
}

void GamePlayScene::StartSequence(std::unique_ptr<GamePlaySequence> sequence, ScenePhase phase)
{
	activeSequence_ = std::move(sequence);
	scenePhase_ = phase;
	activeSequence_->Start();
}

void GamePlayScene::UpdateActiveSequence(float deltaTime)
{
	if (!activeSequence_)
	{
		return;
	}

	const SequenceStatus status = activeSequence_->Update(deltaTime);
	if (status == SequenceStatus::Finished)
	{
		FinishActiveSequence(activeSequence_->GetCompletionAction());
	}
}

void GamePlayScene::FinishActiveSequence(SequenceCompletionAction action)
{
	if (action == SequenceCompletionAction::BeginGameplay)
	{
		activeSequence_.reset();
		scenePhase_ = ScenePhase::Play;
		stageProgressLine_->StartSkydiving();
		player_->FinishStageEntry();
		eventBus_->Dispatch();
		return;
	}

	if (SceneManager::GetInstance()->GetSceneTransition().GetState() == SceneTransition::SceneTransitionState::Idle)
	{
		SceneManager::GetInstance()->SetSceneRequest("TITLE");
	}
}

SequencePolicy GamePlayScene::GetCurrentPolicy() const
{
	if (activeSequence_)
	{
		return activeSequence_->GetPolicy();
	}

	SequencePolicy policy{};
	policy.world = WorldPresentation::GameplayWorld;
	policy.simulation = GameplaySimulation::Running;
	policy.stageProgression = StageProgression::Running;
	policy.playerControl = PlayerControl::Gameplay;
	policy.playerVisibility = PlayerVisibility::Visible;
	policy.gameplayActors = GameplayActorVisibility::Visible;
	policy.hud = HudVisibility::Visible;
	policy.pauseEnabled = true;
	return policy;
}

void GamePlayScene::UpdateStageTitle(float deltaTime)
{
	if (!isStageTitleTimerActive_ || !stageTitleText_)
	{
		return;
	}

	stageTitleTimer_ += deltaTime;
	stageTitleText_->Update();
	if (stageTitleTimer_ >= kStageTitleDelay_ + kStageTitleDuration_)
	{
		isStageTitleTimerActive_ = false;
	}
}

void GamePlayScene::DrawStageTitle()
{
	if (!isStageTitleTimerActive_ || !stageTitleText_ || stageTitleTimer_ < kStageTitleDelay_)
	{
		return;
	}

	stageTitleText_->Draw();
}

void GamePlayScene::StartStageClearSequence()
{
	if (scenePhase_ != ScenePhase::Play || player_->GetIsDead())
	{
		return;
	}

	isPaused_ = false;
	pauseMenu_->SetVisible(false);
	inputMapping_->SetContext(InputContext::Gameplay);
	collisionManager_->Clear();
	BulletManager::GetInstance()->ClearBullets();

	GameClearDependencies dependencies
	{
		*player_,
		*SceneManager::GetInstance()->GetPostEffectController(),
	};
	GameClearConfig config{};
	config.playerTargetPosition = gameplayPlayerRailPosition_;
	StartSequence(std::make_unique<GameClearSequence>(dependencies, config), ScenePhase::Clear);
}

void GamePlayScene::Finalize()
{
	activeSequence_.reset();
	eventSubscriber_.Finalize();

	SceneManager::GetInstance()->GetPostEffectController()->Cancel(bloomHandle_);
	DistanceFogManager::GetInstance()->SetEnabled(false);

	if (pauseMenu_)
	{
		pauseMenu_->Finalize();
		pauseMenu_.reset();
	}
	if (sceneUI_)
	{
		sceneUI_->Finalize();
		sceneUI_.reset();
	}
	if (stageTitleText_)
	{
		stageTitleText_->Finalize();
		stageTitleText_.reset();
	}
	FontManager::GetInstance()->Finalize();
	TextManager::GetInstance()->Finalize();
	FreeTypeManager::GetInstance()->Finalize();

	player_->SetEnemyQueryProvider(nullptr);
	enemyStageDirector_.reset();
	enemyManager_->Finalize();
	player_->Finalize();
	
	debugManager_->Finalize();

	emitters_.clear();

	particleManager_->Reset();

	BulletManager::GetInstance()->Finalize();

	object3ds_.clear();
	stageProgressLine_.reset();
	caveEnvironment_.reset();
	caveRailPath_.reset();
	sprites_.clear();

	camera_->Finalize();
	camera_.reset();

	lightManager_->Finalize();
	cameraManager_->Finalize();
}

void GamePlayScene::Update(const float& deltaTime)
{
	if (inputManager_->TriggerKey(DIK_F11))
	{
		// シーン切り替え
		SceneManager::GetInstance()->SetSceneRequest("TEST");
		return;
	}

	const bool isTransitionIdle =
		SceneManager::GetInstance()->GetSceneTransition().GetState() ==
		SceneTransition::SceneTransitionState::Idle;

	if (isTransitionIdle && inputManager_->TriggerKey(DIK_F1))
	{
		player_->Damage(player_->GetHitPoint());
	}

	if (isTransitionIdle && enemyStageDirector_ && !enemyStageDirector_->IsBossPhase() && 
		inputManager_->TriggerKey(DIK_F2))
	{
		enemyStageDirector_->SkipToBossPhase();
	}

	SequencePolicy policy = GetCurrentPolicy();
	if (policy.pauseEnabled && isTransitionIdle && inputMapping_->IsTriggered(InputAction::Pause))
	{
		isPaused_ = !isPaused_;
		pauseMenu_->SetVisible(isPaused_);
		inputMapping_->SetContext(isPaused_ ? InputContext::Pause : InputContext::Gameplay);
	}

	if (isPaused_)
	{
		const PauseMenu::Action action = pauseMenu_->Update();
		if (isTransitionIdle && action == PauseMenu::Action::ContinueGame)
		{
			isPaused_ = false;
			pauseMenu_->SetVisible(false);
			inputMapping_->SetContext(InputContext::Gameplay);
		}
		else if (isTransitionIdle && action == PauseMenu::Action::ReturnToTitle)
		{
			SceneManager::GetInstance()->SetSceneRequest("TITLE");
		}
		return;
	}

	if (scenePhase_ == ScenePhase::Play && !activeSequence_ &&
		player_->GetIsDead())
	{
		StartGameOverSequence();
		policy = GetCurrentPolicy();
	}

	WorldTransform::AdvanceFrame();
	if (policy.stageProgression == StageProgression::Frozen)
	{
		lightManager_->Update();
		UpdateActiveSequence(deltaTime);
		policy = GetCurrentPolicy();
		if (policy.world == WorldPresentation::GameplayWorld)
		{
			caveEnvironment_->SetEntryMode(true);
			caveEnvironment_->UpdateFog(player_->GetWorldPosition().y);
			caveEnvironment_->UpdateWorld(deltaTime, true);
		}
		player_->Update(deltaTime);
		particleManager_->Update(deltaTime);
		return;
	}

	UpdateGameplayDebugUI();



	/*

	directionalLightData->direction = Vector3{ directionLightDirection.x, directionLightDirection.y, directionLightDirection.z };

	*/


	/*worldMatrixSphere = MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
	wvpMatrixSphere = Multiply(worldMatrixSphere, Multiply(viewMatrix, projectionMatrix));
	transformationMatrixDataSphere->World = worldMatrixSphere;
	transformationMatrixDataSphere->WVP = wvpMatrixSphere;*/



	lightManager_->Update();

	mousePosition_.x = static_cast<float>(inputManager_->MousePoint().x);
	mousePosition_.y = static_cast<float>(inputManager_->MousePoint().y);

	if (isDebugCamera_)
	{
		/*cameraManager_->SetActiveCamera("Debug");*/
	}
	else
	{
		cameraManager_->SetActiveCamera("Default");
	}

	if (stageProgressLine_->Update(
		deltaTime, player_->GetWorldPosition().y))
	{
		stageTitleTimer_ = 0.0f;
		isStageTitleTimerActive_ = true;
	}
	cameraManager_->Update(deltaTime);

	railCameraController_->SetBattleSpaceParent(cameraManager_->GetActiveCameraController()->GetWorldTransform());

	for (auto it = sprites_.begin(); it != sprites_.end(); ++it)
	{
		Sprite* sprite = it->get();
		sprite->Update();
	}

	for (auto it = object3ds_.begin(); it != object3ds_.end(); ++it)
	{
		Object3d* object3d = it->get();
		object3d->Update();
	}

	for (auto it = emitters_.begin(); it != emitters_.end(); ++it)
	{
		ParticleEmitter* emitter = it->get();
		//emitter->Update(deltaTime);
	}

	const bool drawEntryField = stageProgressLine_->IsEntryFieldVisible();
	caveEnvironment_->UpdateWorld(deltaTime, drawEntryField);

	policy = GetCurrentPolicy();
	if (activeSequence_ &&
		policy.simulation == GameplaySimulation::Frozen)
	{
		UpdateActiveSequence(deltaTime);
	}
	player_->Update(deltaTime);
	policy = GetCurrentPolicy();
	if (activeSequence_ && policy.simulation == GameplaySimulation::Running)
	{
		UpdateActiveSequence(deltaTime);
	}
	caveEnvironment_->UpdateWindLines(deltaTime, player_->GetWorldPosition(),
		stageProgressLine_->GetCurrentSpeed(), stageProgressLine_->IsActive());
	UpdateStageTitle(deltaTime);

	policy = GetCurrentPolicy();
	if (policy.playerControl == PlayerControl::Gameplay && inputManager_->TriggerKey(DIK_0))
	{
		slashEmitter_->EmitSlash();
	}

	if (policy.playerControl == PlayerControl::Gameplay && inputManager_->TriggerKey(DIK_1))
	{
		ringEmitter_->Emit();
	}

	if (policy.playerControl == PlayerControl::Gameplay && inputManager_->TriggerKey(DIK_2))
	{
		cylinderEmitter_->Emit();
	}

	if (policy.simulation == GameplaySimulation::Running)
	{
		UpdateGameplaySystems(deltaTime);
	}
	// 敵の移動と衝突によるエフェクト生成後に、最新の親行列で更新する。
	particleManager_->Update(deltaTime);

}

void GamePlayScene::UpdateGameplaySystems(const float& deltaTime)
{
	enemyManager_->SetRailFollowContext(
		caveRailPath_.get(), railCameraController_->GetCurrentDistance(),
		railCameraController_->GetPathWorldOffset());
	enemyStageDirector_->Update(deltaTime);
	const EnemyUpdateContext enemyUpdateContext
	{
		.playerWorldPosition = player_->GetWorldPosition(),
	};
	enemyManager_->Update(deltaTime, enemyUpdateContext);
	BulletManager::GetInstance()->Update(deltaTime);

	collisionManager_->Clear();
	if (!player_->GetIsDead())
	{
		collisionManager_->AddCollider(player_->GetCollider());
		if (player_->GetIsAttackColliderActive())
		{
			collisionManager_->AddCollider(player_->GetAttackCollider());
		}
	}

	for (EnemyID id : enemyManager_->GetEnemyIDs())
	{
		Enemy* enemy = enemyManager_->FindEnemy(id);
		if (!enemy)
		{
			continue;
		}

		collisionManager_->AddCollider(enemy->GetCollider());
		if (enemy->GetIsAttackColliderActive())
		{
			collisionManager_->AddCollider(enemy->GetAttackCollider());
		}
	}

	for (auto& bullet : BulletManager::GetInstance()->GetBullets())
	{
		collisionManager_->AddCollider(bullet->GetCollider());
	}
	collisionManager_->CheckAllCollisions();

	eventBus_->Dispatch();
	sceneUI_->SyncSnapshot(BuildGameUIFrameSnapshot());
	sceneUI_->Update(deltaTime);

}

GameUIFrameSnapshot GamePlayScene::BuildGameUIFrameSnapshot() const
{
	GameUIFrameSnapshot snapshot{};
	snapshot.playerHitPoint.currentHitPoint = player_->GetHitPoint();
	snapshot.playerHitPoint.maxHitPoint = player_->GetMaxHitPoint();

	const std::vector<EnemyID> enemyIDs = enemyManager_->GetEnemyIDs();
	snapshot.enemies.reserve(enemyIDs.size());
	for (const EnemyID id : enemyIDs)
	{
		const Enemy* enemy = enemyManager_->FindEnemy(id);
		if (!enemy)
		{
			continue;
		}

		EnemyUIFrameSnapshot enemySnapshot{};
		enemySnapshot.enemyID = id;
		enemySnapshot.hitPoint.currentHitPoint = enemy->GetHitPoint();
		enemySnapshot.hitPoint.maxHitPoint = enemy->GetMaxHitPoint();
		enemySnapshot.worldPosition = enemy->GetWorldPosition();
		enemySnapshot.isVisible = !enemy->GetIsDead();
		enemySnapshot.displayType = enemy->GetHPGageDisplayType();
		enemySnapshot.screenBossPriority = enemy->GetScreenBossPriority();
		snapshot.enemies.push_back(enemySnapshot);
	}

	const EnemyID lockOnID = player_->GetLockOnEnemyID();
	EnemyTargetInfo lockOnTarget{};
	if (lockOnID != 0 &&
		enemyManager_->TryGetEnemyTarget(lockOnID, lockOnTarget))
	{
		snapshot.isLockOn = true;
		snapshot.targetEnemyID = lockOnID;
		snapshot.targetEnemyWorldPosition = lockOnTarget.worldPosition;
	}

	return snapshot;
}

void GamePlayScene::StartGameOverSequence()
{
	if (scenePhase_ == ScenePhase::GameOver)
	{
		return;
	}

	isPaused_ = false;
	pauseMenu_->SetVisible(false);
	inputMapping_->SetContext(InputContext::Gameplay);

	GameOverDependencies dependencies{
		*player_,
		*railCameraController_,
		*SceneManager::GetInstance()->GetPostEffectController(),
	};

	StartSequence(
		std::make_unique<GameOverSequence>(dependencies),
		ScenePhase::GameOver);
}

void GamePlayScene::UpdateGameplayDebugUI()
{
#ifdef _DEBUG
	ImGui::ShowDemoWindow();

	ImGui::Begin("Setting");
	ImGui::SetWindowSize("Sprite Setting", { 500.0f, 100.0f });
	Vector2 spritePos = sprites_[0]->GetPosition();
	if (ImGui::DragFloat2("pos", &spritePos.x, 1.0f, 0.0f, 0.0f, "%05.1f"))
	{
		sprites_[0]->SetPosition(spritePos);
	}
	ImGui::Checkbox("DrawSprite", &isDrawSprite_);
	ImGui::Checkbox("DrawObject3d", &isDrawObject3d_);
	bool collisionDebugDrawEnabled =
		debugManager_->IsCollisionDebugDrawEnabled();
	if (ImGui::Checkbox("Draw Collision", &collisionDebugDrawEnabled))
	{
		debugManager_->SetCollisionDebugDrawEnabled(collisionDebugDrawEnabled);
	}
	ImGui::End();

	ImGui::Begin("DistanceFogSetting");
	auto* distanceFog = DistanceFogManager::GetInstance();
	bool fogEnabled = distanceFog->IsEnabled();
	float fogStart = distanceFog->GetStartDistance();
	float fogEnd = distanceFog->GetEndDistance();
	Vector4 fogColor = distanceFog->GetColor();
	if (ImGui::Checkbox("EnableDistanceFog", &fogEnabled))
	{
		distanceFog->SetEnabled(fogEnabled);
	}
	if (ImGui::DragFloat("FogStartDistance", &fogStart, 0.5f, 0.0f))
	{
		distanceFog->SetStartDistance(fogStart);
	}
	if (ImGui::DragFloat("FogEndDistance", &fogEnd, 0.5f, 0.0f))
	{
		distanceFog->SetEndDistance(fogEnd);
	}
	if (ImGui::ColorEdit4("FogColor", &fogColor.x))
	{
		distanceFog->SetColor(fogColor);
	}
	ImGui::End();

	ImGui::Begin("LightSetting");
	int32_t enableLighting = object3ds_[0]->GetEnableLighting();
	if (ImGui::Checkbox("EnableLighting", reinterpret_cast<bool*>(&enableLighting)))
	{
		object3ds_[0]->SetEnableLighting(enableLighting);
	}
	Vector3 cameraPos = cameraManager_->GetCameraWorldPosition();
	ImGui::InputFloat3(
		"CameraData", &cameraPos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
	ImGui::End();

	ImGui::Begin("CheckFPS");
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::End();

	if (enemyStageDirector_)
	{
		enemyStageDirector_->DrawDebugUI();
	}
#endif
}

void GamePlayScene::FinishFadeIn()
{
	if (activeSequence_)
	{
		activeSequence_->OnFadeInFinished();
	}
}

void GamePlayScene::Draw(CanvasId canvasId)
{
	const SequencePolicy policy = GetCurrentPolicy();

	if (canvasId == BuiltinCanvas::UI)
	{
		if (policy.hud == HudVisibility::Visible)
		{
			sceneUI_->DrawLayer(UILayer::Screen);
		}
		if (activeSequence_)
		{
			activeSequence_->DrawUI();
		}
		return;
	}

	if (policy.world == WorldPresentation::GameplayWorld)
	{
		const bool drawEntryField =
			policy.stageProgression == StageProgression::Frozen ||
			stageProgressLine_->IsEntryFieldVisible();
		caveEnvironment_->Draw(drawEntryField, isDrawObject3d_);
	}

	if (activeSequence_)
	{
		activeSequence_->DrawWorld();
	}

	const bool drawSequencePlayer =
		policy.world == WorldPresentation::EntryRoom;
	if (policy.playerVisibility == PlayerVisibility::Visible &&
		(isDrawObject3d_ || drawSequencePlayer))
	{
		player_->Draw();
	}
	if (policy.gameplayActors == GameplayActorVisibility::Visible &&
		isDrawObject3d_)
	{
		enemyManager_->Draw();
		BulletManager::GetInstance()->Draw();
	}

	particleManager_->Draw();
	if (activeSequence_)
	{
		activeSequence_->DrawOverlay();
	}

	if (policy.stageProgression == StageProgression::Running && isDrawSprite_)
	{
		for (size_t i = 0; i < sprites_.size(); i++)
		{
			sprites_[i]->Draw();
		}
	}

	if (policy.stageProgression == StageProgression::Running)
	{
		cameraManager_->DrawDebugUI();
	}

#ifdef _DEBUG
	if (policy.stageProgression == StageProgression::Running)
	{
		debugManager_->DrawAll(
			cameraManager_->GetMainCamera()->GetViewProjectionMatrix());
	}
#endif

	if (policy.stageProgression == StageProgression::Running &&
		inputManager_->TriggerKey(DIK_0))
	{
		soundManager_->SoundPlayWave("Alarm01.wav");
	}
	if (policy.stageProgression == StageProgression::Running &&
		inputManager_->TriggerKey(DIK_1))
	{
		soundManager_->SoundPlayWave("test.mp3");

	}

	if (policy.hud == HudVisibility::Visible)
	{
		sceneUI_->DrawLayer(UILayer::World);
		sceneUI_->DrawLayer(UILayer::Overlay);
	}
	if (!activeSequence_)
	{
		DrawStageTitle();
	}

	if (policy.pauseEnabled || isPaused_)
	{
		pauseMenu_->Draw();
	}
}

