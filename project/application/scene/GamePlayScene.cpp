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
		stageEntryPlayerStartPosition_ +
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
		stageEntryPlayerStartPosition_ +
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

	PrimitiveManager::RingConfig stageClearShockwaveConfig;
	stageClearShockwaveConfig.segments = 48;
	stageClearShockwaveConfig.doubleSided = true;
	stageClearShockwaveConfig.innerRadius = 1.5f;
	stageClearShockwaveConfig.outerRadius = 2.0f;
	stageClearShockwaveConfig.innerColor = { 0.0f, 0.4f, 2.0f, 0.8f };
	stageClearShockwaveConfig.outerColor = { 0.0f, 2.0f, 5.0f, 1.0f };
	stageClearShockwaveConfig.uvScaleU = 2.0f;
	stageClearShockwaveConfig.uvScaleV = 0.1f;
	stageClearShockwaveConfig.startAngle = 0.0f;
	stageClearShockwaveConfig.endAngle = 2.0f * pi;
	PrimitiveManager::GetInstance()->CreateRing(
		"stage_clear_shockwave", stageClearShockwaveConfig);
	particleManager_->CreateParticleGroup(
		"stage_clear_shockwave", "gradationLine.png");
	particleManager_->SetModel(
		"stage_clear_shockwave", "stage_clear_shockwave");
	particleManager_->SetIsMoveAccelerationField(
		"stage_clear_shockwave", false);
	particleManager_->SetIsBillboard("stage_clear_shockwave", true);
	particleManager_->SetScaleOverLife(
		"stage_clear_shockwave",
		{ 0.75f, 0.75f, 0.75f }, { 8.0f, 8.0f, 8.0f });
	particleManager_->SetParticleLifeTime("stage_clear_shockwave", 0.5f);

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
	PrimitiveManager::GetInstance()->CreateBox("stage_entry_box");
	particleManager_->CreateParticleGroup(
		kGameplayWindLineGroup_, "white1x1.png");
	particleManager_->SetModel(kGameplayWindLineGroup_, "stage_entry_box");
	particleManager_->SetIsMoveAccelerationField(kGameplayWindLineGroup_, false);
	particleManager_->SetIsBillboard(kGameplayWindLineGroup_, false);
	particleManager_->SetBlendMode(
		kGameplayWindLineGroup_, PSOManager::BlendMode::Normal);

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

	skyBox_ = new SkyBox();
	skyBox_->Initialize();
	skyBox_->SetCamera(camera_.get());

	terrain_ = std::make_unique<Terrain>();
	terrain_->Initialize(Terrain::Mode::GameplayCave, caveRailPath_.get());
	terrain_->SetCaveSpeed(kNormalRailSpeed_);

	player_ = std::make_unique<Player>();
	player_->Initialize(Player::Mode::StageEntry);
	player_->SetEnvironmentTextureIndex(skyBox_->GetEnvironmentTextureIndex());

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

	stageClearShockwaveEmitter_ = std::make_unique<ParticleEmitter>();
	stageClearShockwaveEmitter_->Initialize(
		"stage_clear_shockwave",
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} },
		1, 0.2f);

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
			if (enemyStagePhase_ != EnemyStagePhase::Boss ||
				event.enemyID != bossEnemyID_)
			{
				return;
			}

			auto* controller =
				SceneManager::GetInstance()->GetPostEffectController();
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
			if (enemyStagePhase_ != EnemyStagePhase::Boss ||
				event.enemyID != bossEnemyID_)
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
	InitializeEnemyStage();
	isEnemyStageInitialized_ = true;
	
	eventBus_->Dispatch();

	pauseMenu_ = std::make_unique<PauseMenu>();
	pauseMenu_->Initialize();
	isPaused_ = false;

	InitializeGameplayPresentation();
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
	config.doorPosition = stageEntryDoorPosition_;
	config.playerStartPosition = stageEntryPlayerStartPosition_;
	config.playerDoorPosition = stageEntryPlayerDoorPosition_;
	config.playerDiveEndPosition = stageEntryPlayerDiveEndPosition_;
	config.gameplayPlayerRailPosition = gameplayPlayerRailPosition_;
	config.skydivingStartHeight = skydivingStartHeight_;
	config.skydivingInitialSpeed = skydivingInitialSpeed_;
	config.normalRailSpeed = kNormalRailSpeed_;
	config.blackHoldDuration = kStageEntryBlackHoldDuration_;
	config.approachDuration = kStageEntryApproachDuration_;
	config.doorOpenDuration = kStageEntryDoorOpenDuration_;
	config.diveDuration = kStageEntryDiveDuration_;
	config.whiteOutDuration = kStageEntryWhiteOutDuration_;
	config.whiteFadeDuration = kStageEntryWhiteFadeDuration_;
	config.lookDownForwardHoldDuration = lookDownCameraForwardHoldDuration_;
	config.lookDownTurnDuration = lookDownCameraTurnDuration_;

	activeSequence_ = std::make_unique<StageEntrySequence>(dependencies, config);
	activeSequence_->Start();
	scenePhase_ = ScenePhase::Entry;

	if (!SceneManager::GetInstance()->GetSceneTransition().IsRunning())
	{
		activeSequence_->OnFadeInFinished();
	}
}

void GamePlayScene::UpdateEntrySequence(float deltaTime)
{
	if (!activeSequence_)
	{
		return;
	}

	WorldTransform::AdvanceFrame();
	lightManager_->Update();

	const SequenceStatus status = activeSequence_->Update(deltaTime);
	const SequencePolicy policy = activeSequence_->GetPolicy();
	if (policy.world == WorldPresentation::GameplayWorld)
	{
		terrain_->SetGameplayCaveEntryMode(true);
		UpdateCaveFog();
		skyBox_->Update();
		terrain_->Update(
			deltaTime,
			cameraManager_->GetCameraWorldPosition(),
			railCameraController_->GetCurrentDistance(),
			railCameraController_->GetPathWorldOffset(),
			railCameraController_->GetRailFrameWorldMatrix());
		UpdateCaveFogEndPlane();
		gameplayEntryFieldEntrance_->Update();
		gameplayEntryFieldGround_->Update();
	}

	player_->Update(deltaTime);
	particleManager_->Update(deltaTime);

	if (status == SequenceStatus::Finished)
	{
		FinishEntrySequence();
	}
}

void GamePlayScene::FinishEntrySequence()
{
	activeSequence_.reset();
	scenePhase_ = ScenePhase::Play;
	BeginSkydivingGameplay();
}

void GamePlayScene::InitializeGameplayPresentation()
{
	modelManager_->LoadModel("Field", "FieldEntrance.obj");
	modelManager_->LoadModel("Field", "FieldGround.obj");

	gameplayEntryFieldEntrance_ = std::make_unique<Object3d>();
	gameplayEntryFieldEntrance_->Initialize();
	gameplayEntryFieldEntrance_->SetModel("FieldEntrance.obj");
	gameplayEntryFieldEntrance_->SetRotate({ 0.0f, 0.0f, 0.0f });
	gameplayEntryFieldEntrance_->SetScale(gameplayEntryFieldEntranceScale_);
	gameplayEntryFieldEntrance_->SetTranslate(gameplayEntryFieldEntrancePosition_);

	gameplayEntryFieldGround_ = std::make_unique<Object3d>();
	gameplayEntryFieldGround_->Initialize();
	gameplayEntryFieldGround_->SetModel("FieldGround.obj");
	gameplayEntryFieldGround_->SetRotate({ 0.0f, 0.0f, 0.0f });
	gameplayEntryFieldGround_->SetScale(gameplayEntryFieldGroundScale_);
	gameplayEntryFieldGround_->SetTranslate(gameplayEntryFieldGroundPosition_);

	// Skyboxには距離フォグを適用しないため、視線の先をフォグ色の面で塞ぐ。
	// 姿勢と大きさは毎フレーム、カメラと視錐台に合わせて更新する。
	caveFogEndPlane_ = std::make_unique<Object3d>();
	caveFogEndPlane_->Initialize();
	caveFogEndPlane_->SetModel("plane.obj");
	caveFogEndPlane_->SetEnableLighting(false);
	caveFogEndPlane_->SetScale(
		{ caveFogEndPlaneScale_, caveFogEndPlaneScale_, 1.0f });
	caveFogEndPlane_->SetColor(DistanceFogManager::GetInstance()->GetColor());

	stageTitleText_ = std::make_unique<Text>();
	stageTitleText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf", 64, U"STAGE 1");
	stageTitleText_->SetPosition({ 491.0f, 240.0f });
	stageTitleText_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	stageTitleText_->Update();

	player_->SetScale({ 1.0f, 1.0f, 1.0f });
	player_->SetRotate({ 0.0f, 0.0f, 0.0f });
	player_->SetTranslate(stageEntryPlayerStartPosition_);
	player_->SetTitleOpacity(0.0f);

	gameplayState_ = GameplayState::Inactive;
	gameplayStateTimer_ = 0.0f;
	isEnemyStageInitialized_ = false;
	stageTitleTimer_ = 0.0f;
	isStageTitleTimerActive_ = false;
}

void GamePlayScene::BeginSkydivingGameplay()
{
	gameplayStateTimer_ = 0.0f;
	gameplayState_ = GameplayState::Skydiving;
	railCameraController_->SetMovementEnabled(true);
	ActivateGameplay();
}

void GamePlayScene::UpdateGameplayState(float deltaTime)
{
	switch (gameplayState_)
	{
	case GameplayState::Inactive:
		break;

	case GameplayState::Skydiving:
		UpdateSkydivingSpeed(deltaTime);
		UpdateCaveFog();
		if (caveRailPath_->IsUnderground(
			railCameraController_->GetCurrentDistance()))
		{
			BeginCaveEntering();
		}
		break;

	case GameplayState::CaveEntering:
		UpdateSkydivingSpeed(deltaTime);
		UpdateCaveFog();
		gameplayStateTimer_ += deltaTime;
		if (gameplayStateTimer_ >= kGameplayCaveEnterDuration_)
		{
			BeginNormalGameplay();
		}
		break;

	case GameplayState::Playing:
		break;
	}
}

void GamePlayScene::BeginCaveEntering()
{
	UpdateCaveFog();
	terrain_->SetGameplayCaveEntryMode(false);

	DistanceFogManager::GetInstance()->SetEnabled(true);
	gameplayStateTimer_ = 0.0f;
	gameplayState_ = GameplayState::CaveEntering;
}

void GamePlayScene::BeginNormalGameplay()
{
	gameplayStateTimer_ = 0.0f;
	gameplayState_ = GameplayState::Playing;
	skydivingCurrentSpeed_ = kNormalRailSpeed_;
	railCameraController_->SetMoveSpeed(kNormalRailSpeed_);
	stageTitleTimer_ = 0.0f;
	isStageTitleTimerActive_ = true;
}

void GamePlayScene::UpdateSkydivingSpeed(float deltaTime)
{
	skydivingCurrentSpeed_ += skydivingAcceleration_ * deltaTime;
	skydivingCurrentSpeed_ = std::clamp(
		skydivingCurrentSpeed_, 0.0f, kNormalRailSpeed_);
	railCameraController_->SetMoveSpeed(skydivingCurrentSpeed_);
}

void GamePlayScene::ActivateGameplay()
{
	player_->FinishStageEntry();
	if (!isEnemyStageInitialized_)
	{
		InitializeEnemyStage();
		isEnemyStageInitialized_ = true;
	}
	eventBus_->Dispatch();
}

void GamePlayScene::UpdateCaveFog()
{
	// FogStartは落下中からRailまで基準値20で固定する。
	// FogEndだけ入口までの落下距離を加算し、入口通過後は260へ固定する。
	const float playerHeight = player_->GetWorldPosition().y;
	const float playerHeightAboveEntrance =
		playerHeight > 0.0f ? playerHeight : 0.0f;
	const float fogStartDistance = caveFogBaseStart_;
	const float fogEndDistance =
		caveFogBaseEnd_ + playerHeightAboveEntrance;

	auto* distanceFog = DistanceFogManager::GetInstance();
	distanceFog->SetStartDistance(fogStartDistance);
	distanceFog->SetEndDistance(fogEndDistance);
	distanceFog->SetEnabled(true);
	cameraManager_->GetMainCamera()->SetFarClip(fogEndDistance + 100.0f);
}

void GamePlayScene::UpdateGameplayWindLines(float deltaTime)
{
	const bool isWindActive =
		gameplayState_ != GameplayState::Inactive;
	if (!isWindActive)
	{
		gameplayWindLineEmitAccumulator_ = 0.0f;
		return;
	}

	const bool isOnRail =
		gameplayState_ == GameplayState::CaveEntering ||
		gameplayState_ == GameplayState::Playing;
	const float fallSpeed = isOnRail
		? railCameraController_->GetMoveSpeed()
		: skydivingCurrentSpeed_;
	const float speedRatio = std::clamp(
		fallSpeed / kNormalRailSpeed_,
		0.0f, 1.0f);
	const float emitRate = gameplayWindLineMaxEmitRate_ *
		(0.15f + 0.85f * speedRatio);
	gameplayWindLineEmitAccumulator_ += emitRate * deltaTime;

	auto* random = SeedManager::GetInstance();
	const float minRadius = (std::max)(0.0f, gameplayWindLineMinRadius_);
	const float maxRadius = (std::max)(minRadius, gameplayWindLineMaxRadius_);
	const float minDepth = (std::max)(0.0f, gameplayWindLineMinDepth_);
	const float maxDepth = (std::max)(minDepth, gameplayWindLineMaxDepth_);
	const float minLength = (std::max)(0.1f, gameplayWindLineMinLength_);
	const float maxLength = (std::max)(minLength, gameplayWindLineMaxLength_);
	const float alpha = std::clamp(gameplayWindLineAlpha_, 0.0f, 1.0f);

	while (gameplayWindLineEmitAccumulator_ >= 1.0f)
	{
		gameplayWindLineEmitAccumulator_ -= 1.0f;

		const float angle = random->GenerateFloat(0.0f, 2.0f * pi);
		const float radius = random->GenerateFloat(minRadius, maxRadius);
		const float depth = random->GenerateFloat(minDepth, maxDepth);
		const float width = random->GenerateFloat(0.035f, 0.10f);
		const float baseLength = random->GenerateFloat(minLength, maxLength);
		const float length = baseLength * (0.35f + 0.65f * speedRatio);
		const Vector3 playerPosition = player_->GetWorldPosition();

		EulerTransform transform{};
		transform.scale = { width, length, width };
		transform.rotate = { 0.0f, 0.0f, 0.0f };
		transform.translate =
		{
			playerPosition.x + std::cos(angle) * radius,
			playerPosition.y - depth,
			playerPosition.z + std::sin(angle) * radius,
		};

		const float upwardSpeed =
			20.0f + fallSpeed * 0.25f;
		const float lifeTime = random->GenerateFloat(0.35f, 0.70f);
		particleManager_->EmitParticle(
			kGameplayWindLineGroup_, transform,
			{ 0.0f, upwardSpeed, 0.0f },
			{ 0.72f, 0.88f, 1.0f, alpha }, lifeTime);
	}
}

void GamePlayScene::UpdateCaveFogEndPlane()
{
	if (!caveFogEndPlane_)
	{
		return;
	}

	auto* distanceFog = DistanceFogManager::GetInstance();
	Camera* camera = cameraManager_->GetMainCamera();
	const Vector3 cameraPosition = camera->GetTranslate();
	const float fogEndDistance = distanceFog->GetEndDistance();
	const float planeDistance =
		std::max(fogEndDistance - caveFogEndPlaneOffset_, 0.001f);
	constexpr float kCoverageMargin = 1.05f;
	const float halfHeight =
		std::tan(camera->GetFovY() * 0.5f) * planeDistance * kCoverageMargin;
	const float halfWidth = halfHeight * camera->GetAspectRatio();
	const Quaternion faceCameraRotation = Multiply(
		camera->GetRotateQuat(),
		MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, pi));

	caveFogEndPlane_->SetScale(
		{ std::max(caveFogEndPlaneScale_, halfWidth),
		  std::max(caveFogEndPlaneScale_, halfHeight), 1.0f });
	caveFogEndPlane_->GetWorldTransform()->SetRotateQuat(faceCameraRotation);
	caveFogEndPlane_->SetTranslate(
		cameraPosition + camera->GetForward() * planeDistance);
	caveFogEndPlane_->SetColor(distanceFog->GetColor());
	caveFogEndPlane_->Update();
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
	if (!isStageTitleTimerActive_ || !stageTitleText_ ||
		stageTitleTimer_ < kStageTitleDelay_)
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

	GameClearDependencies dependencies{
		*player_,
		*stageClearShockwaveEmitter_,
		*SceneManager::GetInstance()->GetPostEffectController(),
	};
	GameClearConfig config{};
	config.playerTargetPosition = gameplayPlayerRailPosition_;
	activeSequence_ =
		std::make_unique<GameClearSequence>(dependencies, config);
	activeSequence_->Start();
	scenePhase_ = ScenePhase::Clear;
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
	enemyManager_->Finalize();
	player_->Finalize();
	
	debugManager_->Finalize();

	emitters_.clear();

	particleManager_->Reset();

	BulletManager::GetInstance()->Finalize();

	/*for (auto it = object3ds_.begin(); it != object3ds_.end(); ++it)
	{
		std::unique_ptr<Object3d> object3d = std::move(*it);
		object3d.reset();
	}*/
	object3ds_.clear();
	gameplayEntryFieldEntrance_.reset();
	gameplayEntryFieldGround_.reset();
	caveFogEndPlane_.reset();

	if (terrain_)
	{
		terrain_->Finalize();
		terrain_.reset();
	}
	caveRailPath_.reset();

	skyBox_->Finalize();
	delete skyBox_;

	/*for (auto it = sprites_.begin(); it != sprites_.end(); ++it)
	{
		std::unique_ptr<Sprite> sprite = std::move(*it);
		sprite.reset();
	}*/
	sprites_.clear();


	camera_->Finalize();
	camera_.reset();

	lightManager_->Finalize();
	cameraManager_->Finalize();
	//PostEffectManager::GetInstance()->Clear();
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

	if (isTransitionIdle && enemyStagePhase_ == EnemyStagePhase::SmallEnemies &&
		inputManager_->TriggerKey(DIK_F2))
	{
		const float bossPhaseDistance =
			std::max(0.0f, enemyStageSpawnDelay_) +
			enemyStageConfig_.smallEnemyPhaseDistance;

		railCameraController_->SetTravelledDistance(bossPhaseDistance);
	}

	const bool isGameplayActive = IsStageEntryComplete();
	if (scenePhase_ == ScenePhase::Play &&
		!IsStageClearSequenceActive() && isTransitionIdle &&
		inputMapping_->IsTriggered(InputAction::Pause))
	{
		isPaused_ = !isPaused_;
		pauseMenu_->SetVisible(isPaused_);
		inputMapping_->SetContext(
			isPaused_ ? InputContext::Pause : InputContext::Gameplay);
	}

	if (isGameplayActive && isPaused_)
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

	// StageEntry完了後はゲームオーバーシーケンスを開始できる。
	if (scenePhase_ == ScenePhase::Play &&
		!IsStageClearSequenceActive() && player_->GetIsDead())
	{
		StartGameOverSequence();
	}

	if (!IsStageEntryComplete())
	{
		UpdateEntrySequence(deltaTime);
		return;
	}

	WorldTransform::AdvanceFrame();

#ifdef _DEBUG

	// デモウィンドウ表示
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
	if (ImGui::Checkbox("EnableLighting", (bool*)&enableLighting))
	{
		object3ds_[0]->SetEnableLighting(enableLighting);
	}

	Vector3 cameraPos = cameraManager_->GetCameraWorldPosition();
	ImGui::InputFloat3("CameraData", &cameraPos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

	ImGui::End();

	ImGui::Begin("CheckFPS");

	// FPS表示
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	ImGui::End();

	DrawEnemyStageDebugUI();

#endif



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

	UpdateGameplayState(deltaTime);
	cameraManager_->Update(deltaTime);

	if (gameplayState_ == GameplayState::Skydiving || gameplayState_ == GameplayState::CaveEntering)
	{
		gameplayEntryFieldEntrance_->Update();
		gameplayEntryFieldGround_->Update();
	}

	railCameraController_->SetBattleSpaceParent(
		cameraManager_->GetActiveCameraController()->GetWorldTransform());




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

	skyBox_->Update();

	terrain_->Update(
		deltaTime,
		cameraManager_->GetCameraWorldPosition(),
		railCameraController_->GetCurrentDistance(),
		railCameraController_->GetPathWorldOffset(),
		railCameraController_->GetRailFrameWorldMatrix());
	UpdateCaveFogEndPlane();

	if (scenePhase_ == ScenePhase::Clear && activeSequence_)
	{
		const SequenceStatus status = activeSequence_->Update(deltaTime);
		if (status == SequenceStatus::Finished &&
			SceneManager::GetInstance()->GetSceneTransition().GetState() ==
			SceneTransition::SceneTransitionState::Idle)
		{
			SceneManager::GetInstance()->SetSceneRequest("TITLE");
		}
	}
	player_->Update(deltaTime);
	UpdateGameplayWindLines(deltaTime);
	UpdateStageTitle(deltaTime);

	if (!IsStageClearSequenceActive() && inputManager_->TriggerKey(DIK_0))
	{
		slashEmitter_->EmitSlash();
	}

	if (!IsStageClearSequenceActive() && inputManager_->TriggerKey(DIK_1))
	{
		ringEmitter_->Emit();
	}

	if (!IsStageClearSequenceActive() && inputManager_->TriggerKey(DIK_2))
	{
		cylinderEmitter_->Emit();
	}

	if (!IsStageClearSequenceActive())
	{
		UpdateGameplaySystems(deltaTime);
	}
	// 敵の移動と衝突によるエフェクト生成後に、最新の親行列で更新する。
	particleManager_->Update(deltaTime);
	if (scenePhase_ == ScenePhase::GameOver && activeSequence_)
	{
		const SequenceStatus status = activeSequence_->Update(deltaTime);
		if (status == SequenceStatus::Finished &&
			SceneManager::GetInstance()->GetSceneTransition().GetState() ==
			SceneTransition::SceneTransitionState::Idle)
		{
			SceneManager::GetInstance()->SetSceneRequest("TITLE");
		}
	}

}

void GamePlayScene::UpdateGameplaySystems(const float& deltaTime)
{
	enemyManager_->SetRailFollowContext(
		caveRailPath_.get(), railCameraController_->GetCurrentDistance(),
		railCameraController_->GetPathWorldOffset());
	UpdateEnemyStage(deltaTime);
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

	if (SceneManager::GetInstance()->GetSceneTransition().GetState() !=
		SceneTransition::SceneTransitionState::Idle)
	{
		return;
	}

	if (player_->GetIsDead())
	{
		StartGameOverSequence();
	}
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

	activeSequence_ =
		std::make_unique<GameOverSequence>(dependencies);
	activeSequence_->Start();
	scenePhase_ = ScenePhase::GameOver;
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

	DrawEnemyStageDebugUI();
#endif
}

void GamePlayScene::InitializeEnemyStage()
{
	enemyStageConfig_ = EnemyStageConfig::CreateDefault();
	enemyStageConfig_.Load("enemy/enemy_stage.json");
	enemyStagePhase_ = EnemyStagePhase::SmallEnemies;
	nextSmallEnemyIndex_ = 0;
	bossEnemyID_ = 0;
	smallEnemyPoolHandles_.clear();
	smallEnemyPoolHandles_.reserve(enemyStageConfig_.smallEnemies.size());
	for (const SmallEnemySpawnConfig& spawn : enemyStageConfig_.smallEnemies)
	{
		smallEnemyPoolHandles_.push_back(
			enemyManager_->PrewarmEnemy(spawn.enemy));
	}
	bossEnemyPoolHandle_ = enemyManager_->PrewarmEnemy(enemyStageConfig_.boss);

	// railRatio == 0 の敵も、洞窟内部へ到達するための開始遅延後に出現させる。
	UpdateEnemyStage(0.0f);
}

void GamePlayScene::UpdateEnemyStage(const float& deltaTime)
{
	if (enemyStagePhase_ != EnemyStagePhase::SmallEnemies)
	{
		return;
	}

	static_cast<void>(deltaTime);
	const float phaseDistance = enemyStageConfig_.smallEnemyPhaseDistance;
	const float spawnDelay = enemyStageSpawnDelay_ > 0.0f ? enemyStageSpawnDelay_ : 0.0f;
	const float travelledDistance = railCameraController_->GetTravelledDistance();

	while (nextSmallEnemyIndex_ < enemyStageConfig_.smallEnemies.size())
	{
		const SmallEnemySpawnConfig& spawn =
			enemyStageConfig_.smallEnemies[nextSmallEnemyIndex_];
		const float spawnDistance = spawnDelay + spawn.railRatio * phaseDistance;
		if (travelledDistance < spawnDistance)
		{
			break;
		}
		SpawnSmallEnemy(spawn, nextSmallEnemyIndex_);
		++nextSmallEnemyIndex_;
	}

	if (travelledDistance >= spawnDelay + phaseDistance)
	{
		StartBossPhase();
	}
}

void GamePlayScene::SpawnSmallEnemy(
	const SmallEnemySpawnConfig& spawnConfig, size_t spawnIndex)
{
	EnemyID id = 0;
	if (spawnIndex < smallEnemyPoolHandles_.size())
	{
		id = enemyManager_->ActivatePrewarmedEnemy(
			smallEnemyPoolHandles_[spawnIndex]);
	}
	if (id == 0)
	{
		id = enemyManager_->AddEnemy(spawnConfig.enemy);
	}
	ConfigureEnemy(id, false);
}

void GamePlayScene::StartBossPhase()
{
	// ボス戦では戦闘空間を固定し、洞窟側を流して無限落下を表現する。
	railCameraController_->SetMovementEnabled(false);
	terrain_->SetGameplayCaveScrolling(true);

	enemyManager_->RemoveSmallEnemies();
	bossEnemyID_ =
		enemyManager_->ActivatePrewarmedEnemy(bossEnemyPoolHandle_);
	if (bossEnemyID_ == 0)
	{
		bossEnemyID_ = enemyManager_->AddEnemy(enemyStageConfig_.boss);
	}
	ConfigureEnemy(bossEnemyID_, true);
	enemyStagePhase_ = EnemyStagePhase::Boss;
}

void GamePlayScene::ConfigureEnemy(EnemyID id, bool isBoss)
{
	Enemy* enemy = enemyManager_->FindEnemy(id);
	if (!enemy)
	{
		return;
	}

	enemy->SetEnvironmentTextureIndex(skyBox_->GetEnvironmentTextureIndex());
	if (!isBoss)
	{
		enemy->SetContinuousRailFollowEnabled(
			enemyStageConfig_.smallEnemyContinuousRailFollowEnabled);
	}
	enemy->SetParent(railCameraController_->GetBattleWorldTransform());
	enemy->SetEventBus(eventBus_.get());
	if (isBoss)
	{
		enemy->SetHPGageDisplayType(EnemyHPGageDisplayType::ScreenBoss);
		enemy->SetScreenBossPriority(50);
	}
	else
	{
		enemy->SetHPGageDisplayType(EnemyHPGageDisplayType::OverHead);
	}
	enemy->EventDispatch();
}

void GamePlayScene::DrawEnemyStageDebugUI()
{
#ifdef _DEBUG
	const float travelledDistance = railCameraController_ ?
		railCameraController_->GetTravelledDistance() : 0.0f;
	const float phaseDistance = enemyStageConfig_.smallEnemyPhaseDistance;
	const float spawnDelay = enemyStageSpawnDelay_ > 0.0f ? enemyStageSpawnDelay_ : 0.0f;
	const float progress = phaseDistance > 0.0f ?
		std::clamp((travelledDistance - spawnDelay) / phaseDistance, 0.0f, 1.0f) : 0.0f;

	ImGui::Begin("Enemy Stage Debug");
	ImGui::Text("Rail travelled distance: %.3f", travelledDistance);
	ImGui::Text("Enemy spawn delay: %.3f", spawnDelay);
	ImGui::Text("Small enemy phase distance: %.3f", phaseDistance);
	ImGui::Text("Progress: %.3f / %.1f%%", progress, progress * 100.0f);
	ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f));
	ImGui::Text("Next spawn index: %zu / %zu",
		nextSmallEnemyIndex_, enemyStageConfig_.smallEnemies.size());

	constexpr ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollX;
	if (ImGui::BeginTable("ActiveEnemies", 7, tableFlags, ImVec2(0.0f, 260.0f)))
	{
		ImGui::TableSetupColumn("Enemy ID");
		ImGui::TableSetupColumn("Role");
		ImGui::TableSetupColumn("State");
		ImGui::TableSetupColumn("Spawn Local");
		ImGui::TableSetupColumn("Current Local");
		ImGui::TableSetupColumn("Max HP");
		ImGui::TableSetupColumn("Current HP");
		ImGui::TableHeadersRow();

		for (const EnemyID id : enemyManager_->GetEnemyIDs())
		{
			const Enemy* enemy = enemyManager_->FindEnemy(id);
			if (!enemy || enemy->GetIsDead())
			{
				continue;
			}

			const Vector3& spawnPosition = enemy->GetSpawnPosition();
			const Vector3& currentPosition = enemy->GetTranslate();
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%u", static_cast<unsigned int>(id));
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(enemy->GetRoleName());
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(enemy->GetBehaviorStateName());
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("(%.2f, %.2f, %.2f)",
				spawnPosition.x, spawnPosition.y, spawnPosition.z);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("(%.2f, %.2f, %.2f)",
				currentPosition.x, currentPosition.y, currentPosition.z);
			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%d", enemy->GetMaxHitPoint());
			ImGui::TableSetColumnIndex(6);
			ImGui::Text("%d", enemy->GetHitPoint());
		}
		ImGui::EndTable();
	}
	ImGui::End();
#endif
}

void GamePlayScene::FinishFadeIn()
{
	if (scenePhase_ == ScenePhase::Entry && activeSequence_)
	{
		activeSequence_->OnFadeInFinished();
	}
}

void GamePlayScene::Draw(CanvasId canvasId)
{
	if (canvasId == BuiltinCanvas::UI)
	{
		if (scenePhase_ == ScenePhase::Clear && activeSequence_)
		{
			if (activeSequence_->GetPolicy().hud == HudVisibility::Visible)
			{
				sceneUI_->DrawLayer(UILayer::Screen);
			}
			activeSequence_->DrawUI();
			return;
		}

		if (scenePhase_ == ScenePhase::GameOver && activeSequence_)
		{
			activeSequence_->DrawUI();
			return;
		}

		if (scenePhase_ == ScenePhase::Entry && activeSequence_)
		{
			activeSequence_->DrawUI();
			return;
		}

		// 画面固定UIはポストエフェクト適用後に合成する。
		// HPゲージと操作説明をWorld Canvasへ描くと、ステージクリア時の
		// ラジアルブラーに巻き込まれるため、UI Canvasへ分離する。
		if (IsStageEntryComplete())
		{
			sceneUI_->DrawLayer(UILayer::Screen);
		}
		return;
	}

	if (scenePhase_ == ScenePhase::Entry && activeSequence_)
	{
		const SequencePolicy policy = activeSequence_->GetPolicy();
		if (policy.world == WorldPresentation::GameplayWorld)
		{
			skyBox_->Draw();
			caveFogEndPlane_->Draw();
			terrain_->Draw();
			DrawGameplayEntryField();
		}

		activeSequence_->DrawWorld();
		if (policy.playerVisibility == PlayerVisibility::Visible)
		{
			player_->Draw();
		}
		if (policy.gameplayActors == GameplayActorVisibility::Visible)
		{
			enemyManager_->Draw();
			BulletManager::GetInstance()->Draw();
		}

		particleManager_->Draw();
		activeSequence_->DrawOverlay();
		return;
	}

	skyBox_->Draw();

	if (isDrawObject3d_)
	{
		/*for (size_t i = 0; i < object3ds_.size(); i++)
		{
			object3ds_[i]->Draw();
		}*/

		caveFogEndPlane_->Draw();
		terrain_->Draw();
		if (gameplayState_ == GameplayState::Skydiving ||
			gameplayState_ == GameplayState::CaveEntering)
		{
			DrawGameplayEntryField();
		}
		player_->Draw();
		/*enemy_->Draw();*/
		enemyManager_->Draw();
		BulletManager::GetInstance()->Draw();
	}

	
	


	particleManager_->Draw();
	if (scenePhase_ == ScenePhase::Clear && activeSequence_)
	{
		activeSequence_->DrawWorld();
		activeSequence_->DrawOverlay();
	}
	if (scenePhase_ == ScenePhase::GameOver && activeSequence_)
	{
		activeSequence_->DrawWorld();
		activeSequence_->DrawOverlay();
	}

	if (isDrawSprite_)
	{
		for (size_t i = 0; i < sprites_.size(); i++)
		{
			sprites_[i]->Draw();
		}
	}

	cameraManager_->DrawDebugUI();

#ifdef _DEBUG
	debugManager_->DrawAll(cameraManager_->GetMainCamera()->GetViewProjectionMatrix());
#endif

	if (inputManager_->TriggerKey(DIK_0))
	{
		soundManager_->SoundPlayWave("Alarm01.wav");
	}
	if (inputManager_->TriggerKey(DIK_1))
	{
		soundManager_->SoundPlayWave("test.mp3");

	}

	sceneUI_->DrawLayer(UILayer::World);
	sceneUI_->DrawLayer(UILayer::Overlay);
	DrawStageTitle();

	pauseMenu_->Draw();
}

void GamePlayScene::DrawGameplayEntryField()
{
	gameplayEntryFieldGround_->Draw();
	gameplayEntryFieldEntrance_->Draw();
}
