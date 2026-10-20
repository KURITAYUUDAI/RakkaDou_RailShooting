#include "GamePlayScene.h"
#include "SceneManager.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "PrimitiveManager.h"
#include "PostEffectManager.h"
#include "GaussianBlur.h"
#include "RadialBlur.h"
#include "Dissolve.h"
#include "Random.h"
#include "Bloom.h"
#include "Grayscale.h"
#include "BulletManager.h"
#include "SplineCurve.h"
#include "HPGageUI.h"
#include "DistanceFogManager.h"
#include "freetype/FreeTypeManager.h"
#include "FontManager.h"
#include "TextManager.h"

#include <algorithm>
#include <cfloat>

namespace
{
	float SmoothStep01(float value)
	{
		const float t = std::clamp(value, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	float LerpFloat(float start, float end, float t)
	{
		return start + (end - start) * t;
	}

	Vector3 LerpVector3(const Vector3& start, const Vector3& end, float t)
	{
		return {
			LerpFloat(start.x, end.x, t),
			LerpFloat(start.y, end.y, t),
			LerpFloat(start.z, end.z, t),
		};
	}
}

void GamePlayScene::Initialize()
{
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
	camera_->SetTranslate({ 0.0f, 1.0f, -13.0f });
	camera_->SetFarClip(300.0f);

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
	defaultCameraController_->GetWorldTransform()->translate_ = { 0.0f, 1.0f, -13.0f };
	defaultCameraController_->GetWorldTransform()->SetRotate({ 0.0f, 0.0f, 0.0f });

	railCameraController_ = std::make_unique<RailCameraController>();
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

	lightManager_->SetNumPointLights(1);
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
	lightManager_->SetSpotLightCosFalloff(0, std::cosf(pi / 4.0f));

	auto* distanceFog = DistanceFogManager::GetInstance();
	distanceFog->SetStartDistance(20.0f);
	distanceFog->SetEndDistance(260.0f);
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
	PrimitiveManager::GetInstance()->CreateBox("stage_entry_box");

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
	terrain_->Initialize(Terrain::Mode::GameplayCave);
	terrain_->SetCaveSpeed(kStageEntryNormalRailSpeed_);

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

	//PostEffectManager::GetInstance()->Add("Outline");
	//PostEffectManager::GetInstance()->Add("GaussianBlur");
	//PostEffectManager::GetInstance()->Add("RadialBlur");
	//PostEffectManager::GetInstance()->Add("Bloom");
	/*PostEffectManager::GetInstance()->Add("Dissolve");*/
	/*PostEffectManager::GetInstance()->Add("Random");*/

	bloomHandle_ = SceneManager::GetInstance()->GetPostEffectController()->Emit<Bloom>("Bloom", std::nullopt, nullptr);
	//glayscaleHandle_ = SceneManager::GetInstance()->GetPostEffectController()->Emit<Grayscale>("Grayscale", std::nullopt, nullptr);

	debugManager_->Initialize();

	// シーン初期化終わり

	soundManager_->SoundLoadFile("", "Alarm01.wav");
	soundManager_->SoundLoadFile("", "test.mp3");

	FreeTypeManager::GetInstance()->Initialize();
	TextManager::GetInstance()->Initialize();

	eventBus_ = std::make_unique<EventBus>();
	sceneUI_ = std::make_unique<GamePlaySceneUI>();
	sceneUI_->Initialize(eventBus_.get());

	player_->SetEventBus(eventBus_.get());
	player_->EventDispatch();

	enemyManager_->Initialize(eventBus_.get());
	
	eventBus_->Dispatch();

	pauseMenu_ = std::make_unique<PauseMenu>();
	pauseMenu_->Initialize();
	isPaused_ = false;

	InitializeStageEntry();
}

void GamePlayScene::InitializeStageEntry()
{
	modelManager_->LoadModel("Field", "FieldEntrance.obj");
	modelManager_->LoadModel("Field", "FieldGround.obj");

	const auto createBox = [this](const Vector3& scale, const Vector3& translate,
		const Vector4& color)
		{
			auto object = std::make_unique<Object3d>();
			object->Initialize();
			object->SetModel("stage_entry_box");
			object->SetEnableLighting(false);
			object->SetScale(scale);
			object->SetTranslate(translate);
			object->SetColor(color);
			return object;
		};

	stageEntryBackdrop_ = createBox(
		{ 40.0f, 24.0f, 0.5f }, { 0.0f, 0.0f, 10.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f });
	stageEntryLight_ = createBox(
		{ 8.0f, 10.0f, 0.15f }, { 0.0f, 0.0f, 8.4f },
		{ 8.0f, 8.0f, 8.0f, 1.0f });
	stageEntryLeftDoor_ = createBox(
		{ 4.0f, 10.0f, 0.35f }, { -2.0f, 0.0f, 8.0f },
		{ 0.015f, 0.02f, 0.035f, 1.0f });
	stageEntryRightDoor_ = createBox(
		{ 4.0f, 10.0f, 0.35f }, { 2.0f, 0.0f, 8.0f },
		{ 0.015f, 0.02f, 0.035f, 1.0f });

	stageEntryDoorFrames_[0] = createBox(
		{ 0.6f, 11.0f, 0.6f }, { -4.3f, 0.0f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });
	stageEntryDoorFrames_[1] = createBox(
		{ 0.6f, 11.0f, 0.6f }, { 4.3f, 0.0f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });
	stageEntryDoorFrames_[2] = createBox(
		{ 9.2f, 0.6f, 0.6f }, { 0.0f, 5.3f, 7.9f },
		{ 0.12f, 0.14f, 0.2f, 1.0f });

	stageEntryFieldEntrance_ = std::make_unique<Object3d>();
	stageEntryFieldEntrance_->Initialize();
	stageEntryFieldEntrance_->SetModel("FieldEntrance.obj");
	stageEntryFieldEntrance_->SetRotate({ 0.0f, 0.0f, 0.0f });
	stageEntryFieldEntrance_->SetScale(stageEntryFieldEntranceScale_);
	stageEntryFieldEntrance_->SetTranslate(stageEntryFieldEntrancePosition_);

	stageEntryFieldGround_ = std::make_unique<Object3d>();
	stageEntryFieldGround_->Initialize();
	stageEntryFieldGround_->SetModel("FieldGround.obj");
	stageEntryFieldGround_->SetRotate({ 0.0f, 0.0f, 0.0f });
	stageEntryFieldGround_->SetScale(stageEntryFieldGroundScale_);
	stageEntryFieldGround_->SetTranslate(stageEntryFieldGroundPosition_);

	// Skyboxには距離フォグを適用しないため、洞窟の最深部をフォグ色の面で塞ぐ。
	// plane.objはXY平面なので、上空から見える水平面になるようX軸に-90度回転する。
	caveFogEndPlane_ = std::make_unique<Object3d>();
	caveFogEndPlane_->Initialize();
	caveFogEndPlane_->SetModel("plane.obj");
	caveFogEndPlane_->SetEnableLighting(false);
	caveFogEndPlane_->SetRotate({ -pi / 2.0f, 0.0f, 0.0f });
	caveFogEndPlane_->SetScale(
		{ caveFogEndPlaneScale_, caveFogEndPlaneScale_, 1.0f });
	caveFogEndPlane_->SetColor(DistanceFogManager::GetInstance()->GetColor());

	stageEntryWhiteSprite_ = std::make_unique<Sprite>();
	stageEntryWhiteSprite_->Initialize("white1x1.png");
	stageEntryWhiteSprite_->SetSize({
		static_cast<float>(WindowsAPI::kClientWidth),
		static_cast<float>(WindowsAPI::kClientHeight) });
	stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

	player_->SetScale({ 1.0f, 1.0f, 1.0f });
	player_->SetRotate({ 0.0f, 0.0f, 0.0f });
	player_->SetTranslate({ 0.0f, 0.0f, -7.0f });
	player_->SetTitleOpacity(0.0f);

	stageEntryDoorProgress_ = 0.0f;
	stageEntryTimer_ = 0.0f;
	isStageEntryFieldVisible_ = false;
	isStageEntryRailActive_ = false;
	isEnemyStageInitialized_ = false;
	stageEntryState_ = SceneManager::GetInstance()->GetSceneTransition().IsRunning()
		? StageEntryState::WaitingForFadeIn
		: StageEntryState::BlackHold;

	UpdateStageEntryObjects();
}

void GamePlayScene::UpdateStageEntry(const float& deltaTime)
{
#ifdef _DEBUG
	ImGui::Begin("StageEntry");
	ImGui::Text("State: %d", static_cast<int>(stageEntryState_));
	ImGui::Text("Timer: %.3f", stageEntryTimer_);
	ImGui::DragFloat3(
		"EntranceScale", &stageEntryFieldEntranceScale_.x, 0.01f, 0.01f, 10.0f);
	ImGui::DragFloat3(
		"EntrancePosition", &stageEntryFieldEntrancePosition_.x, 0.1f);
	ImGui::DragFloat3(
		"GroundScale", &stageEntryFieldGroundScale_.x, 0.01f, 0.01f, 10.0f);
	ImGui::DragFloat3(
		"GroundPosition", &stageEntryFieldGroundPosition_.x, 0.1f);
	ImGui::Separator();
	ImGui::DragFloat(
		"SkydivingStartHeight", &stageEntrySkydivingStartHeight_, 1.0f, 100.0f, 3000.0f);
	ImGui::DragFloat(
		"SkydivingInitialSpeed", &stageEntrySkydivingInitialSpeed_, 0.5f, 0.0f,
		kStageEntryNormalRailSpeed_);
	ImGui::DragFloat(
		"SkydivingAcceleration", &stageEntrySkydivingAcceleration_, 0.5f, 0.0f, 300.0f);
	ImGui::Text("CurrentFallSpeed: %.2f / %.2f",
		stageEntrySkydivingCurrentSpeed_, kStageEntryNormalRailSpeed_);
	ImGui::DragFloat(
		"CameraForwardHold", &stageEntryCameraForwardHoldDuration_, 0.01f, 0.0f, 3.0f);
	ImGui::DragFloat(
		"CameraTurnDuration", &stageEntryCameraTurnDuration_, 0.01f, 0.01f, 5.0f);
	ImGui::DragFloat(
		"CaveEntryStartHeight", &stageEntryCaveEntryStartHeight_, 0.5f, 0.0f, 300.0f);
	ImGui::DragFloat(
		"RailConnectHeight", &stageEntryRailConnectHeight_, 0.5f, -300.0f, 0.0f);
	ImGui::DragFloat(
		"CameraHeightOffset", &stageEntryCameraHeightOffset_, 0.1f, 1.0f, 100.0f);
	ImGui::DragFloat(
		"CaveFogBaseStart", &stageEntryCaveFogBaseStart_, 0.5f, 0.0f, 1000.0f);
	ImGui::DragFloat(
		"CaveFogBaseEnd", &stageEntryCaveFogBaseEnd_, 0.5f, 0.0f, 1000.0f);
	ImGui::Text("CaveFog: %.1f -> %.1f",
		DistanceFogManager::GetInstance()->GetStartDistance(),
		DistanceFogManager::GetInstance()->GetEndDistance());
	ImGui::DragFloat(
		"FogEndPlaneScale", &caveFogEndPlaneScale_, 0.5f, 1.0f, 200.0f);
	ImGui::DragFloat(
		"FogEndPlaneOffset", &caveFogEndPlaneOffset_, 0.1f, -100.0f, 100.0f);
	ImGui::End();
#endif

	switch (stageEntryState_)
	{
	case StageEntryState::WaitingForFadeIn:
		break;

	case StageEntryState::BlackHold:
		stageEntryTimer_ += deltaTime;
		if (stageEntryTimer_ >= kStageEntryBlackHoldDuration_)
		{
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::Approaching;
		}
		break;

	case StageEntryState::Approaching:
	{
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / kStageEntryApproachDuration_, 0.0f, 1.0f);
		const float easedProgress = SmoothStep01(progress);
		const Vector3 playerPosition = LerpVector3(
			{ 0.0f, 0.0f, -7.0f }, { 0.0f, 0.0f, 1.5f }, easedProgress);
		player_->SetTranslate(playerPosition);
		player_->SetTitleOpacity(std::clamp(progress / 0.35f, 0.0f, 1.0f));
		defaultCameraController_->GetWorldTransform()->translate_ =
			{ 0.0f, 1.0f, playerPosition.z - 6.0f };

		if (progress >= 1.0f)
		{
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::OpeningDoor;
		}
		break;
	}

	case StageEntryState::OpeningDoor:
	{
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / kStageEntryDoorOpenDuration_, 0.0f, 1.0f);
		stageEntryDoorProgress_ = SmoothStep01(progress);
		if (progress >= 1.0f)
		{
			StartStageEntryBlur();
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::Diving;
		}
		break;
	}

	case StageEntryState::Diving:
	{
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / kStageEntryDiveDuration_, 0.0f, 1.0f);
		const float easedProgress = progress * progress;
		const Vector3 playerPosition = LerpVector3(
			{ 0.0f, 0.0f, 1.5f }, { 0.0f, 0.0f, 10.5f }, easedProgress);
		player_->SetTranslate(playerPosition);
		defaultCameraController_->GetWorldTransform()->translate_ =
			{ 0.0f, 1.0f, LerpFloat(-4.5f, 5.5f, easedProgress) };
		UpdateStageEntryBlur(easedProgress);

		if (progress >= 1.0f)
		{
			StopStageEntryBlur();
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::WhiteOut;
		}
		break;
	}

	case StageEntryState::WhiteOut:
	{
		stageEntryTimer_ += deltaTime;
		const float progress = std::clamp(
			stageEntryTimer_ / kStageEntryWhiteOutDuration_, 0.0f, 1.0f);
		stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, SmoothStep01(progress) });
		if (progress >= 1.0f)
		{
			BeginStageEntrySkydiving();
		}
		break;
	}

	case StageEntryState::Skydiving:
	{
		stageEntryTimer_ += deltaTime;
		stageEntryCameraTurnTimer_ += deltaTime;
		stageEntrySkydivingCurrentSpeed_ +=
			stageEntrySkydivingAcceleration_ * deltaTime;
		if (stageEntrySkydivingCurrentSpeed_ > kStageEntryNormalRailSpeed_)
		{
			stageEntrySkydivingCurrentSpeed_ = kStageEntryNormalRailSpeed_;
		}
		Vector3 playerPosition = player_->GetTranslate();
		playerPosition.y -= stageEntrySkydivingCurrentSpeed_ * deltaTime;
		player_->SetTranslate(playerPosition);
		UpdateStageEntryCaveFog();
		UpdateStageEntrySkydivingCamera();

		const float whiteProgress = std::clamp(
			stageEntryTimer_ / kStageEntryWhiteFadeDuration_, 0.0f, 1.0f);
		stageEntryWhiteSprite_->SetColor(
			{ 1.0f, 1.0f, 1.0f, 1.0f - SmoothStep01(whiteProgress) });

		if (playerPosition.y <= stageEntryCaveEntryStartHeight_)
		{
			stageEntryTimer_ = 0.0f;
			stageEntryState_ = StageEntryState::CaveEntry;
			DistanceFogManager::GetInstance()->SetEnabled(true);
		}
		break;
	}

	case StageEntryState::CaveEntry:
	{
		stageEntryTimer_ += deltaTime;
		stageEntryCameraTurnTimer_ += deltaTime;
		stageEntrySkydivingCurrentSpeed_ +=
			stageEntrySkydivingAcceleration_ * deltaTime;
		if (stageEntrySkydivingCurrentSpeed_ > kStageEntryNormalRailSpeed_)
		{
			stageEntrySkydivingCurrentSpeed_ = kStageEntryNormalRailSpeed_;
		}
		Vector3 playerPosition = player_->GetTranslate();
		playerPosition.y -= stageEntrySkydivingCurrentSpeed_ * deltaTime;
		player_->SetTranslate(playerPosition);
		UpdateStageEntryCaveFog();
		UpdateStageEntrySkydivingCamera();

		const float configuredEntryDistance =
			stageEntryCaveEntryStartHeight_ - stageEntryRailConnectHeight_;
		const float entryDistance =
			configuredEntryDistance > 0.001f ? configuredEntryDistance : 0.001f;
		const float progress = std::clamp(
			(stageEntryCaveEntryStartHeight_ - playerPosition.y) / entryDistance,
			0.0f, 1.0f);
		player_->SetRotate(
			{ LerpFloat(pi / 2.0f, 0.0f, SmoothStep01(progress)), 0.0f, 0.0f });

		if (playerPosition.y <= stageEntryRailConnectHeight_)
		{
			playerPosition.y = stageEntryRailConnectHeight_;
			player_->SetTranslate(playerPosition);
			BeginStageEntryRailConnection();
		}
		break;
	}

	case StageEntryState::Connecting:
		stageEntryTimer_ += deltaTime;
		if (stageEntryTimer_ >= kStageEntryConnectDuration_)
		{
			CompleteStageEntry();
		}
		break;

	case StageEntryState::Complete:
		return;
	}

	cameraManager_->Update(deltaTime);
	if (isStageEntryFieldVisible_)
	{
		if (isStageEntryRailActive_)
		{
			railCameraController_->SetBattleSpaceParent(
				cameraManager_->GetActiveCameraController()->GetWorldTransform());
		}
		skyBox_->Update();
		terrain_->Update(deltaTime, cameraManager_->GetCameraWorldPosition());
		UpdateCaveFogEndPlane();
	}

	UpdateStageEntryObjects();
	player_->Update(deltaTime);
	stageEntryWhiteSprite_->Update();
}

void GamePlayScene::BeginStageEntrySkydiving()
{
	StopStageEntryBlur();
	isStageEntryFieldVisible_ = true;
	isStageEntryRailActive_ = false;
	stageEntryTimer_ = 0.0f;
	stageEntryCameraTurnTimer_ = 0.0f;
	stageEntryState_ = StageEntryState::Skydiving;
	stageEntrySkydivingCurrentSpeed_ = stageEntrySkydivingInitialSpeed_;
	if (stageEntrySkydivingCurrentSpeed_ < 0.0f)
	{
		stageEntrySkydivingCurrentSpeed_ = 0.0f;
	}
	else if (stageEntrySkydivingCurrentSpeed_ > kStageEntryNormalRailSpeed_)
	{
		stageEntrySkydivingCurrentSpeed_ = kStageEntryNormalRailSpeed_;
	}

	cameraManager_->SetActiveCameraController("Default");
	terrain_->SetGameplayCaveEntryMode(true);
	DistanceFogManager::GetInstance()->SetEnabled(true);
	player_->SetTranslate({ 1.0f, stageEntrySkydivingStartHeight_, -2.0f });
	player_->SetRotate({ 0.0f, 0.0f, 0.0f });
	UpdateStageEntryCaveFog();
	UpdateStageEntrySkydivingCamera();
}

void GamePlayScene::BeginStageEntryRailConnection()
{
	const Vector3 playerWorldPosition = player_->GetTranslate();
	UpdateStageEntryCaveFog();
	terrain_->SetGameplayCaveEntryMode(false);
	const float railStartHeight =
		playerWorldPosition.y + stageEntryCameraHeightOffset_;

	railCameraController_->SetVerticalStartHeight(railStartHeight);
	railCameraController_->SetMovementEnabled(true);
	railCameraController_->SetMoveSpeed(kStageEntryNormalRailSpeed_);
	cameraManager_->SetActiveCameraController("Rail");
	railCameraController_->SetBattleSpaceParent(
		cameraManager_->GetActiveCameraController()->GetWorldTransform());

	player_->SetTranslate({ 0.0f, -3.0f, -7.5f });
	player_->SetRotate({ 0.0f, 0.0f, 0.0f });
	player_->SetParent(railCameraController_->GetBattleWorldTransform());

	DistanceFogManager::GetInstance()->SetEnabled(true);
	isStageEntryRailActive_ = true;
	stageEntryTimer_ = 0.0f;
	stageEntryState_ = StageEntryState::Connecting;
}

void GamePlayScene::UpdateStageEntrySkydivingCamera()
{
	const Vector3& playerPosition = player_->GetTranslate();
	const float turnDuration =
		stageEntryCameraTurnDuration_ > 0.001f ? stageEntryCameraTurnDuration_ : 0.001f;
	const float turnProgress = std::clamp(
		(stageEntryCameraTurnTimer_ - stageEntryCameraForwardHoldDuration_) / turnDuration,
		0.0f, 1.0f);
	const float easedProgress = SmoothStep01(turnProgress);
	const Vector3 cameraOffset = LerpVector3(
		{ 0.0f, 1.0f, -6.0f },
		{ 0.0f, stageEntryCameraHeightOffset_, 3.0f },
		easedProgress);
	auto* cameraTransform = defaultCameraController_->GetWorldTransform();
	cameraTransform->translate_ = playerPosition + cameraOffset;
	cameraTransform->SetRotate(
		{ LerpFloat(0.0f, pi / 2.0f, easedProgress), 0.0f, 0.0f });
	if (stageEntryState_ == StageEntryState::Skydiving)
	{
		player_->SetRotate(
			{ LerpFloat(0.0f, pi / 2.0f, easedProgress), 0.0f, 0.0f });
	}
}

void GamePlayScene::UpdateStageEntryCaveFog()
{
	// FogStartは落下中からRailまで基準値20で固定する。
	// FogEndだけ入口までの落下距離を加算し、入口通過後は260へ固定する。
	const float playerHeight = player_->GetTranslate().y;
	const float playerHeightAboveEntrance =
		playerHeight > 0.0f ? playerHeight : 0.0f;
	const float fogStartDistance = stageEntryCaveFogBaseStart_;
	const float fogEndDistance =
		stageEntryCaveFogBaseEnd_ + playerHeightAboveEntrance;

	auto* distanceFog = DistanceFogManager::GetInstance();
	distanceFog->SetStartDistance(fogStartDistance);
	distanceFog->SetEndDistance(fogEndDistance);
	distanceFog->SetEnabled(true);
	cameraManager_->GetMainCamera()->SetFarClip(fogEndDistance + 100.0f);
}

void GamePlayScene::UpdateCaveFogEndPlane()
{
	if (!caveFogEndPlane_)
	{
		return;
	}

	auto* distanceFog = DistanceFogManager::GetInstance();
	const Vector3 cameraPosition = cameraManager_->GetCameraWorldPosition();
	const float fogEndDistance = distanceFog->GetEndDistance();

	caveFogEndPlane_->SetScale(
		{ caveFogEndPlaneScale_, caveFogEndPlaneScale_, 1.0f });
	caveFogEndPlane_->SetTranslate(
		{ 0.0f, cameraPosition.y - fogEndDistance + caveFogEndPlaneOffset_, 0.0f });
	caveFogEndPlane_->SetColor(distanceFog->GetColor());
	caveFogEndPlane_->Update();
}

void GamePlayScene::CompleteStageEntry()
{
	stageEntryWhiteSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	isStageEntryFieldVisible_ = false;
	railCameraController_->SetMoveSpeed(kStageEntryNormalRailSpeed_);
	railCameraController_->ResetTravelledDistance();
	player_->FinishStageEntry();

	if (!isEnemyStageInitialized_)
	{
		InitializeEnemyStage();
		isEnemyStageInitialized_ = true;
		eventBus_->Dispatch();
	}

	stageEntryTimer_ = 0.0f;
	stageEntryState_ = StageEntryState::Complete;
}

void GamePlayScene::UpdateStageEntryObjects()
{
	constexpr float doorHalfWidth = 2.0f;
	constexpr float leftHingeX = -4.0f;
	constexpr float rightHingeX = 4.0f;
	constexpr float doorZ = 8.0f;
	const float leftAngle = -stageEntryDoorProgress_ * (pi / 2.0f);
	const float rightAngle = stageEntryDoorProgress_ * (pi / 2.0f);

	stageEntryLeftDoor_->SetRotate({ 0.0f, leftAngle, 0.0f });
	stageEntryLeftDoor_->SetTranslate({
		leftHingeX + std::cos(leftAngle) * doorHalfWidth,
		0.0f,
		doorZ - std::sin(leftAngle) * doorHalfWidth });
	stageEntryRightDoor_->SetRotate({ 0.0f, rightAngle, 0.0f });
	stageEntryRightDoor_->SetTranslate({
		rightHingeX - std::cos(rightAngle) * doorHalfWidth,
		0.0f,
		doorZ + std::sin(rightAngle) * doorHalfWidth });

	stageEntryBackdrop_->Update();
	stageEntryLight_->Update();
	stageEntryLeftDoor_->Update();
	stageEntryRightDoor_->Update();
	for (auto& frame : stageEntryDoorFrames_)
	{
		frame->Update();
	}

	if (isStageEntryFieldVisible_)
	{
		stageEntryFieldEntrance_->SetScale(stageEntryFieldEntranceScale_);
		stageEntryFieldEntrance_->SetTranslate(stageEntryFieldEntrancePosition_);
		stageEntryFieldGround_->SetScale(stageEntryFieldGroundScale_);
		stageEntryFieldGround_->SetTranslate(stageEntryFieldGroundPosition_);
		stageEntryFieldEntrance_->Update();
		stageEntryFieldGround_->Update();
	}
}

void GamePlayScene::StartStageEntryBlur()
{
	auto* controller = SceneManager::GetInstance()->GetPostEffectController();
	if (controller->IsActive(stageEntryBlurHandle_))
	{
		return;
	}

	stageEntryBlurHandle_ = controller->Emit<RadialBlur>(
		"RadialBlur", std::nullopt, nullptr);
	controller->Edit<RadialBlur>(
		stageEntryBlurHandle_,
		[](RadialBlur& blur)
		{
			blur.SetCenter({ 0.5f, 0.5f });
			blur.SetBlurWidth(0.0f);
		});
}

void GamePlayScene::UpdateStageEntryBlur(float progress)
{
	auto* controller = SceneManager::GetInstance()->GetPostEffectController();
	if (!controller->IsActive(stageEntryBlurHandle_))
	{
		return;
	}

	const float blurWidth = 0.02f * std::clamp(progress, 0.0f, 1.0f);
	controller->Edit<RadialBlur>(
		stageEntryBlurHandle_,
		[blurWidth](RadialBlur& blur)
		{
			blur.SetBlurWidth(blurWidth);
		});
}

void GamePlayScene::StopStageEntryBlur()
{
	if (stageEntryBlurHandle_ == kInvalidPostEffectHandle)
	{
		return;
	}

	SceneManager::GetInstance()->GetPostEffectController()->Cancel(stageEntryBlurHandle_);
	stageEntryBlurHandle_ = kInvalidPostEffectHandle;
}

void GamePlayScene::Finalize()
{
	StopStageEntryBlur();
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
	FontManager::GetInstance()->Finalize();
	TextManager::GetInstance()->Finalize();
	FreeTypeManager::GetInstance()->Finalize();

	enemyManager_->Finalize();
	player_->Finalize();
	
	debugManager_->Finalize();

	//PostEffectManager::GetInstance()->Clear();

	/*for (auto it = emitters_.begin(); it != emitters_.end(); ++it)
	{
		std::unique_ptr<ParticleEmitter> emitter = std::move(*it);
		emitter.reset();

	}*/
	emitters_.clear();

	particleManager_->Reset();

	BulletManager::GetInstance()->Finalize();

	/*for (auto it = object3ds_.begin(); it != object3ds_.end(); ++it)
	{
		std::unique_ptr<Object3d> object3d = std::move(*it);
		object3d.reset();
	}*/
	object3ds_.clear();
	stageEntryBackdrop_.reset();
	stageEntryLight_.reset();
	stageEntryLeftDoor_.reset();
	stageEntryRightDoor_.reset();
	for (auto& frame : stageEntryDoorFrames_)
	{
		frame.reset();
	}
	stageEntryFieldEntrance_.reset();
	stageEntryFieldGround_.reset();
	caveFogEndPlane_.reset();
	stageEntryWhiteSprite_.reset();

	if (terrain_)
	{
		terrain_->Finalize();
		terrain_.reset();
	}

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
	//if (inputManager_->TriggerKey(DIK_RETURN))
	//{
	//	// シーン切り替え
	//	SceneManager::GetInstance()->SetSceneRequest("TITLE");
	//}

	const bool isTransitionIdle =
		SceneManager::GetInstance()->GetSceneTransition().GetState() ==
		SceneTransition::SceneTransitionState::Idle;

	if (isTransitionIdle && inputManager_->TriggerKey(DIK_F1))
	{
		player_->Damage(player_->GetHitPoint());
		SceneManager::GetInstance()->SetSceneRequest("RESULT");
		return;
	}

	if (!IsStageEntryComplete())
	{
		WorldTransform::AdvanceFrame();
		lightManager_->Update();
		UpdateStageEntry(deltaTime);
		return;
	}

	if (isTransitionIdle && inputManager_->TriggerKey(DIK_TAB))
	{
		isPaused_ = !isPaused_;
		pauseMenu_->SetVisible(isPaused_);
	}

	if (isPaused_)
	{
		const PauseMenu::Action action = pauseMenu_->Update();
		if (isTransitionIdle && action == PauseMenu::Action::ContinueGame)
		{
			isPaused_ = false;
			pauseMenu_->SetVisible(false);
		}
		else if (isTransitionIdle && action == PauseMenu::Action::ReturnToTitle)
		{
			SceneManager::GetInstance()->SetSceneRequest("TITLE");
		}
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


	/*ImGui::Begin("GaussianBlur");
	auto* blur = PostEffectManager::GetInstance()->Get<GaussianBlur>("GaussianBlur");
	float sigma = blur->GetSigma();
	int kernelRadius = blur->GetKernelRadius();
	if (ImGui::SliderFloat("Sigma", &sigma, 0.1f, 20.0f))
	{
		blur->SetSigma(sigma);
	}
	if (ImGui::SliderInt("KernelRadius", &kernelRadius, 1, 64))
	{
		blur->SetKernelRadius(kernelRadius);
	}

	ImGui::End();*/

	/*ImGui::Begin("RadialBlur");
	auto* blur = PostEffectManager::GetInstance()->Get<RadialBlur>("RadialBlur");
	Vector2 center = blur->GetCenter();
	float blurWidth = blur->GetBlurWidth();
	if (ImGui::SliderFloat2("center", &center.x, 0.0f, 1.0f))
	{
		blur->SetCenter(center);
	}
	if (ImGui::SliderFloat("BlurWidth", &blurWidth, 0.0f, 0.01f))
	{
		blur->SetBlurWidth(blurWidth);
	}

	ImGui::End();*/

	/*ImGui::Begin("Dissolve");
	auto* dissolve = PostEffectManager::GetInstance()->Get<Dissolve>("Dissolve");
	Vector4 edgeColor = dissolve->GetEdgeColor();
	float threshold = dissolve->GetThreshold();
	if (ImGui::ColorEdit4("##colorSprite", &edgeColor.x))
	{
		dissolve->SetEdgeColor(edgeColor);
	}
	if (ImGui::SliderFloat("threshold", &threshold, 0.0f, 1.0f))
	{
		dissolve->SetThreshold(threshold);
	}

	ImGui::End();*/

	//auto* random = PostEffectManager::GetInstance()->Get<Random>("Random");
	//random->SetTime()

	//ImGui_ImplDX12_NewFrame();
	//ImGui_ImplWin32_NewFrame();
	//ImGui::NewFrame();

	
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

	cameraManager_->Update(deltaTime);
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

	particleManager_->Update(deltaTime);

	skyBox_->Update();

	terrain_->Update(deltaTime, cameraManager_->GetCameraWorldPosition());
	UpdateCaveFogEndPlane();

	player_->Update(deltaTime);
	// 敵の接近先計算で、同じフレームのPlayer位置を使えるよう先に配信する。
	eventBus_->Dispatch();

	UpdateEnemyStage(deltaTime);
	/*enemy_->Update(deltaTime);*/
	enemyManager_->Update(deltaTime);

	BulletManager::GetInstance()->Update(deltaTime);

	if (inputManager_->TriggerKey(DIK_0))
	{
		slashEmitter_->EmitSlash();
	}

	if (inputManager_->TriggerKey(DIK_1))
	{
		ringEmitter_->Emit();
	}

	if (inputManager_->TriggerKey(DIK_2))
	{
		cylinderEmitter_->Emit();
	}


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
		if (enemyManager_->FindEnemy(id))
		{
			Enemy* enemy = enemyManager_->FindEnemy(id);
			collisionManager_->AddCollider(enemy->GetCollider());
			if (enemy->GetIsAttackColliderActive())
			{
				collisionManager_->AddCollider(enemy->GetAttackCollider());
			}
		}
	}
	
	/*for (auto& enemy : EnemyManager::GetInstance()->GetEnemies())
	{
		collisionManager_->AddCollider(enemy->GetCollider());
	}*/

	for (auto& bullet : BulletManager::GetInstance()->GetBullets())
	{
		collisionManager_->AddCollider(bullet->GetCollider());
	}
	collisionManager_->CheckAllCollisions();

	//// ImGuiの内部コマンドを生成する
	//ImGui::Render();


	eventBus_->Dispatch();
	sceneUI_->Update(deltaTime);

	if (SceneManager::GetInstance()->GetSceneTransition().GetState() == SceneTransition::SceneTransitionState::Idle)
	{
		if (enemyStagePhase_ == EnemyStagePhase::Boss && bossEnemyID_ != 0 &&
			enemyManager_->GetIsDead(bossEnemyID_))
		{
			SceneManager::GetInstance()->SetSceneRequest("RESULT");
		}

		if (player_->GetIsDead())
		{
			SceneManager::GetInstance()->SetSceneRequest("RESULT");
		}
	}

}

void GamePlayScene::InitializeEnemyStage()
{
	enemyStageConfig_ = EnemyStageConfig::CreateDefault();
	enemyStageConfig_.Load("enemy/enemy_stage.json");
	enemyStagePhase_ = EnemyStagePhase::SmallEnemies;
	nextSmallEnemyIndex_ = 0;
	bossEnemyID_ = 0;

	// railRatio == 0 の敵はゲーム開始時に出現させる。
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
	const float travelledDistance = railCameraController_->GetTravelledDistance();

	while (nextSmallEnemyIndex_ < enemyStageConfig_.smallEnemies.size())
	{
		const SmallEnemySpawnConfig& spawn =
			enemyStageConfig_.smallEnemies[nextSmallEnemyIndex_];
		const float spawnDistance = spawn.railRatio * phaseDistance;
		if (travelledDistance < spawnDistance)
		{
			break;
		}
		SpawnSmallEnemy(spawn);
		++nextSmallEnemyIndex_;
	}

	if (travelledDistance >= phaseDistance)
	{
		StartBossPhase();
	}
}

void GamePlayScene::SpawnSmallEnemy(const SmallEnemySpawnConfig& spawnConfig)
{
	const EnemyID id = enemyManager_->AddEnemy(spawnConfig.enemy);
	ConfigureEnemy(id, false);
}

void GamePlayScene::StartBossPhase()
{
	// ボス戦では戦闘空間を固定し、洞窟側を流して無限落下を表現する。
	railCameraController_->SetMovementEnabled(false);
	terrain_->SetGameplayCaveScrolling(true);

	enemyManager_->RemoveSmallEnemies();
	bossEnemyID_ = enemyManager_->AddEnemy(enemyStageConfig_.boss);
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
	const float progress = phaseDistance > 0.0f ?
		std::clamp(travelledDistance / phaseDistance, 0.0f, 1.0f) : 0.0f;

	ImGui::Begin("Enemy Stage Debug");
	ImGui::Text("Rail travelled distance: %.3f", travelledDistance);
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
	if (stageEntryState_ == StageEntryState::WaitingForFadeIn)
	{
		stageEntryTimer_ = 0.0f;
		stageEntryState_ = StageEntryState::BlackHold;
	}
}

void GamePlayScene::Draw()
{
	if (!IsStageEntryComplete())
	{
		if (isStageEntryFieldVisible_)
		{
			DrawStageEntryField();
		}
		else
		{
			DrawStageEntryDarkSpace();
		}

		if (stageEntryWhiteSprite_ && stageEntryWhiteSprite_->GetColor().w > 0.0f)
		{
			stageEntryWhiteSprite_->Draw();
		}
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
		player_->Draw();
		/*enemy_->Draw();*/
		enemyManager_->Draw();
		BulletManager::GetInstance()->Draw();
	}

	
	


	particleManager_->Draw();

	/*for (size_t i = 0; i < particle3ds.size(); i++)
	{
		particle3ds[i]->Draw();
	}*/

	//// SphereのIndexBufferViewを設定
	//commandList->IASetIndexBuffer(&indexBufferView);
	// 
	//// Sphereの描画
	//dxBase->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);	// VBVを設定
	//// SphereのIndexBufferViewを設定
	//dxBase->GetCommandList()->IASetIndexBuffer(&indexBufferViewSphere);
	//// SphereのマテリアルのCBufferの場所を設定
	//dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSphere->GetGPUVirtualAddress());
	//// SphereのTransformationMatrixCBufferの場所を設定
	//dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationResourceSphere->GetGPUVirtualAddress());
	//// SRV用のDescriptorTableの先頭を設定。2はrootParameter[2]である。
	//dxBase->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandles[0]);	// SphereはCheckerを使う
	//// 平行光源用のCBufferをバインド（rootParameter[3] = b1）
	//dxBase->GetCommandList()->SetGraphicsRootConstantBufferView(3, DirectionalLightResource->GetGPUVirtualAddress());
	//// 描画！（DrawCall/ドローコール）。
	//dxBase->GetCommandList()->DrawIndexedInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0, 0);


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
	sceneUI_->DrawLayer(UILayer::Screen);
	sceneUI_->DrawLayer(UILayer::Overlay);

	pauseMenu_->Draw();
}

void GamePlayScene::DrawStageEntryDarkSpace()
{
	stageEntryBackdrop_->Draw();
	if (stageEntryState_ == StageEntryState::WaitingForFadeIn ||
		stageEntryState_ == StageEntryState::BlackHold)
	{
		return;
	}

	stageEntryLight_->Draw();
	stageEntryLeftDoor_->Draw();
	stageEntryRightDoor_->Draw();
	for (auto& frame : stageEntryDoorFrames_)
	{
		frame->Draw();
	}
	player_->Draw();
}

void GamePlayScene::DrawStageEntryField()
{
	skyBox_->Draw();
	caveFogEndPlane_->Draw();
	terrain_->Draw();
	stageEntryFieldGround_->Draw();
	stageEntryFieldEntrance_->Draw();
	player_->Draw();
}
