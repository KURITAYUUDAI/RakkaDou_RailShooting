#include "TitleScene.h"
#include "SceneManager.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "DistanceFogManager.h"
#include "RadialBlur.h"
#include "Random.h"

#include "freetype/FreeTypeManager.h"
#include "FontManager.h"
#include "TextManager.h"

#include <algorithm>

namespace
{
	float SmoothStep01(float value)
	{
		const float t = std::clamp(value, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	Vector3 LerpVector3(const Vector3& start, const Vector3& end, float t)
	{
		return {
			start.x + (end.x - start.x) * t,
			start.y + (end.y - start.y) * t,
			start.z + (end.z - start.z) * t,
		};
	}
}

void TitleScene::Initialize()
{
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();

	// Textureを読んで転送する
	textureManager_->LoadTexture("uvChecker.png");
	textureManager_->LoadTexture("monsterBall.png");
	textureManager_->LoadTexture("title.png");

	int textureIndex = 0;
	const char* textureOptions[] = { "Checker", "monsterBall" };

	// シーン初期化始め
	/*camera_->SetRotate({ pi / 3.0f, pi, 0.0f });
	camera_->SetTranslate({ 0.0f, 9.0f, 5.0f });*/
	camera_->SetRotate(entranceCameraSideRotation_);
	camera_->SetTranslate(entranceCameraStartPosition_);


	/*object3dManager_->SetDefaultCamera(camera_.get());
	particleManager_->SetDefaultCamera(camera_.get());*/

	//debugCamera_ = std::make_unique<DebugCamera>();
	//debugCamera_->Initialize();
	//debugCamera_->SetRotate(camera_->GetRotate());
	//debugCamera_->SetTranslate(camera_->GetTranslate());

	cameraManager_->Initialize();
	// Object3dのWVP生成に使われるのはCameraManagerのmainCamera_。
	// 洞窟の最奥セグメントまで正しい深度値を確保する。
	cameraManager_->GetMainCamera()->SetFarClip(300.0f);
	cameraManager_->AddCamera("Default", camera_.get());
	/*cameraManager_->AddCamera("Debug", debugCamera_.get());*/
	cameraManager_->SetActiveCamera("Default");

	defaultCameraController_ = std::make_unique<DefaultCameraController>();
	defaultCameraController_->Initialize();
	defaultCameraController_->GetWorldTransform()->translate_ = entranceCameraStartPosition_;
	defaultCameraController_->GetWorldTransform()->SetRotate(entranceCameraSideRotation_);

	cameraManager_->AddCameraController("Default", defaultCameraController_.get());
	cameraManager_->SetActiveCameraController("Default");

	lightManager_->Initialize();
	lightManager_->SetDirectionalLightColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	lightManager_->SetDirectionalLightDirection({ 0.0f, -1.0f, 0.0f });
	lightManager_->SetDirectionalLightIntensity(1.0f);

	auto* distanceFog = DistanceFogManager::GetInstance();
	distanceFog->SetStartDistance(20.0f);
	distanceFog->SetEndDistance(260.0f);
	distanceFog->SetColor({ 0.005f, 0.008f, 0.015f, 1.0f });
	distanceFog->SetEnabled(true);

	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->Initialize();
	skyBox_->SetCamera(camera_.get());

	terrain_ = std::make_unique<Terrain>();
	terrain_->Initialize(Terrain::Mode::TitleCave);
	terrain_->SetCaveSpeed(kTitleCaveSpeed_);

	for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<Sprite> newSprite = std::make_unique<Sprite>();
		newSprite->Initialize("uvChecker.png");
		newSprite->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		sprites_.push_back(std::move(newSprite));
	}

	sprites_[0]->SetTexture("title.png");
	sprites_[0]->SetPosition(Vector2{ 640.0f, 200.0f });
	sprites_[0]->AdjustTextureSize();
	sprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });



	modelManager_->LoadModel("", "plane.obj");
	modelManager_->LoadModel("", "axis.obj");
	modelManager_->LoadModel("", "sphere.obj");

	// Skyboxをフォグ対象外にしたまま、洞窟中央の最深部だけをフォグ色で塞ぐ。
	caveFogEndPlane_ = std::make_unique<Object3d>();
	caveFogEndPlane_->Initialize();
	caveFogEndPlane_->SetModel("plane.obj");
	caveFogEndPlane_->SetEnableLighting(false);
	caveFogEndPlane_->SetRotate({ -pi / 2.0f, 0.0f, 0.0f });
	caveFogEndPlane_->SetScale(
		{ caveFogEndPlaneScale_, caveFogEndPlaneScale_, 1.0f });
	caveFogEndPlane_->SetColor(distanceFog->GetColor());

	for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<Object3d> newObject3d = std::make_unique<Object3d>();
		newObject3d->Initialize();
		newObject3d->SetModel("plane.obj");
		object3ds_.push_back(std::move(newObject3d));
	}

	object3ds_[0]
		->SetModel("sphere.obj");


	particleManager_->SetModel("plane.obj");
	//particleManager_->CreateParticleGroup("circle", "circle.png");


	/*AABB  aabb;
	aabb.max = { 1.0f, 1.0f, 1.0f };
	aabb.min = { -1.0f, -1.0f, -1.0f };
	particleManager_->CreateAccelerationField({ 5.0f, 0.0f, 0.0f }, aabb);*/


	/*for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<ParticleEmitter> emitter = std::make_unique<ParticleEmitter>();
		EulerTransform transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
		emitter->Initialize("circle", transform, 3, 0.2f);
		emitters_.push_back(std::move(emitter));
	}*/

	openingFadeSprite_ = std::make_unique<Sprite>();
	openingFadeSprite_->Initialize("white1x1.png");
	openingFadeSprite_->SetSize({
		static_cast<float>(WindowsAPI::kClientWidth),
		static_cast<float>(WindowsAPI::kClientHeight) });
	openingFadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

	PrimitiveManager::GetInstance()->Initialize();
	PrimitiveManager::GetInstance()->CreateBox("title_wind_line_box");
	particleManager_->CreateParticleGroup(kWindLineGroup_, "white1x1.png");
	particleManager_->SetModel(kWindLineGroup_, "title_wind_line_box");
	particleManager_->SetIsMoveAccelerationField(kWindLineGroup_, false);
	particleManager_->SetIsBillboard(kWindLineGroup_, false);
	particleManager_->SetBlendMode(
		kWindLineGroup_, PSOManager::BlendMode::Normal);
	windLineEmitAccumulator_ = 0.0f;

	PrimitiveManager::RingConfig ringConfig;
	ringConfig.segments = 48;
	ringConfig.doubleSided = true;
	ringConfig.innerRadius = 1.5f;
	ringConfig.outerRadius = 2.0f;
	ringConfig.innerColor = { 0.0f, 0.4f, 2.0f, 0.8f };
	ringConfig.outerColor = { 0.0f, 2.0f, 5.0f, 1.0f };
	ringConfig.uvScaleU = 2.0f;
	ringConfig.uvScaleV = 0.1f;
	ringConfig.startAngle = 0.0f;
	ringConfig.endAngle = 2.0f * pi;
	PrimitiveManager::GetInstance()->CreateRing("title_shockwave", ringConfig);

	particleManager_->CreateParticleGroup("title_shockwave", "gradationLine.png");
	particleManager_->SetModel("title_shockwave", "title_shockwave");
	particleManager_->SetIsMoveAccelerationField("title_shockwave", false);
	particleManager_->SetIsBillboard("title_shockwave", true);
	particleManager_->SetScaleOverLife(
		"title_shockwave", { 0.75f, 0.75f, 0.75f }, { 8.0f, 8.0f, 8.0f });
	particleManager_->SetParticleLifeTime("title_shockwave", 0.5f);

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Initialize("title_shockwave",
		{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 1, 0.2f);

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

	cylinderEmitter_ = std::make_unique<ParticleEmitter>();
	cylinderEmitter_->Initialize("cylinder",
		{ {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 1, 0.2f);



	PSOManager::GetInstance()->RegisterEnvironmentPSO();

	player_ = std::make_unique<Player>();
	player_->Initialize(Player::Mode::TitleFalling);
	player_->SetEnvironmentTextureIndex(skyBox_->GetEnvironmentTextureIndex());
	entrancePlayerFinalPosition_ = player_->GetTranslate();
	entrancePlayerStartPosition_ = entrancePlayerFinalPosition_;
	entrancePlayerStartPosition_.y += entrancePlayerStartHeight_;
	player_->SetTranslate(entrancePlayerStartPosition_);
	player_->SetTitleOpacity(0.0f);
	entranceSequenceState_ = EntranceSequenceState::PlayerFalling;
	entranceSequenceTimer_ = 0.0f;
	exitSequenceState_ = ExitSequenceState::WaitingForInput;
	exitSequenceTimer_ = 0.0f;
	exitStartPlayerPosition_ = player_->GetTranslate();

	// シーン初期化終わり

	SoundManager::GetInstance()->SoundLoadFile("", "Alarm01.wav");
	SoundManager::GetInstance()->SoundLoadFile("", "test.mp3");

	FreeTypeManager::GetInstance()->Initialize();
	TextManager::GetInstance()->Initialize();

	spacePromptText_ = std::make_unique<Text>();
	spacePromptText_->Initialize(
		"fonts/x8y12pxTheStrongGamer.ttf", 48, U"SPACE");
	spacePromptText_->SetPosition({ 562.0f, 580.0f });
	spacePromptText_->SetColor({ 0.9f, 0.9f, 0.9f, 1.0f });


	debugManager_->Initialize();
}

void TitleScene::Finalize()
{
	StopExitBlur();

	// 次のシーンへタイトル専用フォグを持ち越さない。
	DistanceFogManager::GetInstance()->SetEnabled(false);

	if (player_)
	{
		player_->Finalize();
		player_.reset();
	}
	if (terrain_)
	{
		terrain_->Finalize();
		terrain_.reset();
	}
	if (skyBox_)
	{
		skyBox_->Finalize();
		skyBox_.reset();
	}
	caveFogEndPlane_.reset();

	for (auto it = emitters_.begin(); it != emitters_.end(); ++it)
	{
		std::unique_ptr<ParticleEmitter> emitter = std::move(*it);
		emitter.reset();

	}
	emitters_.clear();
	particleManager_->Reset();

	for (auto it = object3ds_.begin(); it != object3ds_.end(); ++it)
	{
		std::unique_ptr<Object3d> object3d = std::move(*it);
		object3d.reset();
	}
	object3ds_.clear();

	for (auto it = sprites_.begin(); it != sprites_.end(); ++it)
	{
		std::unique_ptr<Sprite> sprite = std::move(*it);
		sprite.reset();
	}
	sprites_.clear();

	camera_->Finalize();
	camera_.reset();

	lightManager_->Finalize();
	cameraManager_->Finalize();
	debugManager_->Finalize();

	FreeTypeManager::GetInstance()->Finalize();
	FontManager::GetInstance()->Finalize();
	TextManager::GetInstance()->Finalize();
	//PostEffectManager::GetInstance()->Clear();
}

void TitleScene::Update(const float& deltaTime)
{
	if (inputManager_->TriggerKey(DIK_F1) &&
		!SceneManager::GetInstance()->GetSceneTransition().IsRunning())
	{
		SceneManager::GetInstance()->SetSceneRequest("GAMEPLAY");
		return;
	}

	WorldTransform::AdvanceFrame();
	bool skippedEntranceThisFrame = false;
	if (entranceSequenceState_ != EntranceSequenceState::Complete &&
		exitSequenceState_ == ExitSequenceState::WaitingForInput &&
		inputManager_->TriggerKey(DIK_SPACE))
	{
		SkipEntranceSequence();
		skippedEntranceThisFrame = true;
	}
	else
	{
		UpdateEntranceSequence(deltaTime);
	}

	// スキップに使ったSPACEを、同じフレームのゲーム開始入力には流用しない。
	if (!skippedEntranceThisFrame)
	{
		UpdateExitSequence(deltaTime);
	}

#ifdef USE_IMGUI

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
	ImGui::Checkbox("DebugCamera", &isDebugCamera_);
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
	ImGui::DragFloat(
		"FogEndPlaneScale", &caveFogEndPlaneScale_, 0.5f, 1.0f, 200.0f);
	ImGui::DragFloat(
		"FogEndPlaneOffset", &caveFogEndPlaneOffset_, 0.1f, -100.0f, 100.0f);
	ImGui::End();

	ImGui::Begin("Title Exit Sequence");
	ImGui::Text("Entrance State: %d", static_cast<int>(entranceSequenceState_));
	ImGui::DragFloat("Opening Fade Duration", &openingFadeDuration_, 0.05f, 0.0f, 5.0f);
	ImGui::DragFloat("Player Fall Duration", &entranceFallDuration_, 0.05f, 0.01f, 10.0f);
	ImGui::DragFloat("Camera Wait Duration", &entranceCameraDelay_, 0.05f, 0.0f, 10.0f);
	ImGui::DragFloat("Camera Move Duration", &entranceCameraMoveDuration_, 0.05f, 0.01f, 10.0f);
	ImGui::DragFloat("Title Fade Duration", &titleFadeDuration_, 0.05f, 0.01f, 10.0f);
	ImGui::Separator();
	ImGui::DragFloat("Title Fade Out Duration", &titleFadeOutDuration_, 0.01f, 0.01f, 2.0f);
	ImGui::DragFloat("Player Move Duration", &exitMoveDuration_, 0.05f, 0.01f, 10.0f);
	ImGui::DragFloat("Player Move Distance", &exitMoveDistance_, 1.0f, 1.0f, 1000.0f);
	ImGui::DragFloat("Dissolve Delay", &dissolveDelay_, 0.05f, 0.0f, 10.0f);
	ImGui::End();

	ImGui::Begin("LightSetting");

	int32_t enableLighting = object3ds_[0]->GetEnableLighting();
	Vector4 directionLightColor = LightManager::GetInstance()->GetDirectionalLightColor();
	Vector3 directionLightDirection = LightManager::GetInstance()->GetDirectionalLightDirection();
	float directionLightIntensity = LightManager::GetInstance()->GetDirectionalLightIntensity();
	if (ImGui::Checkbox("EnableLighting", (bool*)&enableLighting))
	{
		object3ds_[0]->SetEnableLighting(enableLighting);
	}
	if (ImGui::ColorEdit4("LightColor", &directionLightColor.x))
	{
		LightManager::GetInstance()->SetDirectionalLightColor(directionLightColor);
	}
	if (ImGui::DragFloat3("LightDirection", &directionLightDirection.x, 0.01f))
	{
		LightManager::GetInstance()->SetDirectionalLightDirection(Normalize(directionLightDirection));
	}
	if (ImGui::DragFloat("LightIntensity", &directionLightIntensity, 0.01f))
	{
		LightManager::GetInstance()->SetDirectionalLightIntensity(directionLightIntensity);
	}

	Vector3 cameraPos = CameraManager::GetInstance()->GetCameraWorldPosition();
	ImGui::InputFloat3("CameraData", &cameraPos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

	ImGui::End();

#endif



	/*

	directionalLightData->direction = Vector3{ directionLightDirection.x, directionLightDirection.y, directionLightDirection.z };

	*/


	/*worldMatrixSphere = MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
	wvpMatrixSphere = Multiply(worldMatrixSphere, Multiply(viewMatrix, projectionMatrix));
	transformationMatrixDataSphere->World = worldMatrixSphere;
	transformationMatrixDataSphere->WVP = wvpMatrixSphere;*/

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
	skyBox_->Update();
	terrain_->Update(deltaTime);
	UpdateCaveFogEndPlane();



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

	/*for (auto it = emitters_.begin(); it != emitters_.end(); ++it)
	{
		ParticleEmitter* emitter = it->get();
		emitter->Update(deltaTime);
	}*/

	if (inputManager_->TriggerKey(DIK_1))
	{
		ringEmitter_->Emit();
	}

	if (inputManager_->TriggerKey(DIK_2))
	{
		cylinderEmitter_->Emit();
	}

	player_->Update(deltaTime);
	UpdateWindLines(deltaTime);

	openingFadeSprite_->Update();

	particleManager_->Update(deltaTime);

	//// ImGuiの内部コマンドを生成する
	//ImGui::Render();

	spacePromptText_->Update();
}

void TitleScene::UpdateCaveFogEndPlane()
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

void TitleScene::UpdateWindLines(float deltaTime)
{
	windLineEmitAccumulator_ += kWindLineEmitRate_ * deltaTime;

	auto* random = SeedManager::GetInstance();
	while (windLineEmitAccumulator_ >= 1.0f)
	{
		windLineEmitAccumulator_ -= 1.0f;

		const float angle = random->GenerateFloat(0.0f, 2.0f * pi);
		const float radius = random->GenerateFloat(
			kWindLineMinRadius_, kWindLineMaxRadius_);
		const float depth = random->GenerateFloat(
			kWindLineMinDepth_, kWindLineMaxDepth_);
		const float width = random->GenerateFloat(0.035f, 0.10f);
		const float length = random->GenerateFloat(
			kWindLineMinLength_, kWindLineMaxLength_);
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

		const float upwardSpeed = 20.0f + kTitleCaveSpeed_ * 0.25f;
		const float lifeTime = random->GenerateFloat(0.35f, 0.70f);
		particleManager_->EmitParticle(
			kWindLineGroup_, transform,
			{ 0.0f, upwardSpeed, 0.0f },
			{ 0.72f, 0.88f, 1.0f, kWindLineAlpha_ }, lifeTime);
	}
}

void TitleScene::UpdateEntranceSequence(float deltaTime)
{
	switch (entranceSequenceState_)
	{
	case EntranceSequenceState::PlayerFalling:
	{
		entranceSequenceTimer_ += deltaTime;
		const float fallDuration =
			entranceFallDuration_ > 0.001f ? entranceFallDuration_ : 0.001f;
		const float progress = std::clamp(entranceSequenceTimer_ / fallDuration, 0.0f, 1.0f);
		const float fallProgress = progress * progress;
		const float fadeProgress = SmoothStep01(progress);

		player_->SetTranslate(LerpVector3(
			entrancePlayerStartPosition_, entrancePlayerFinalPosition_, fallProgress));
		player_->SetTitleOpacity(fadeProgress);
		SetTitleCameraTransform(
			LerpVector3(entranceCameraStartPosition_, entranceCameraSidePosition_, progress),
			entranceCameraSideRotation_);

		const float fadeDuration = openingFadeDuration_ > 0.001f ? openingFadeDuration_ : 0.001f;
		const float screenFadeProgress =
			std::clamp(entranceSequenceTimer_ / fadeDuration, 0.0f, 1.0f);
		openingFadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f - screenFadeProgress });

		if (progress >= 1.0f)
		{
			player_->SetTranslate(entrancePlayerFinalPosition_);
			player_->SetTitleOpacity(1.0f);
			SetTitleCameraTransform(
				entranceCameraSidePosition_, entranceCameraSideRotation_);
			openingFadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
			entranceSequenceTimer_ = 0.0f;
			entranceSequenceState_ = EntranceSequenceState::WaitingForCamera;
		}
		break;
	}

	case EntranceSequenceState::WaitingForCamera:
	{
		entranceSequenceTimer_ += deltaTime;
		const float cameraDelay = entranceCameraDelay_ > 0.0f ? entranceCameraDelay_ : 0.0f;
		if (entranceSequenceTimer_ >= cameraDelay)
		{
			entranceSequenceTimer_ = 0.0f;
			entranceSequenceState_ = EntranceSequenceState::MovingCamera;
		}
		break;
	}

	case EntranceSequenceState::MovingCamera:
	{
		entranceSequenceTimer_ += deltaTime;
		const float moveDuration =
			entranceCameraMoveDuration_ > 0.001f ? entranceCameraMoveDuration_ : 0.001f;
		const float progress = std::clamp(entranceSequenceTimer_ / moveDuration, 0.0f, 1.0f);
		const float cameraProgress = SmoothStep01(progress);

		SetTitleCameraTransform(
			LerpVector3(entranceCameraSidePosition_, entranceCameraFinalPosition_, cameraProgress),
			LerpVector3(entranceCameraSideRotation_, entranceCameraFinalRotation_, cameraProgress));

		if (progress >= 1.0f)
		{
			SetTitleCameraTransform(
				entranceCameraFinalPosition_, entranceCameraFinalRotation_);
			entranceSequenceTimer_ = 0.0f;
			entranceSequenceState_ = EntranceSequenceState::FadingTitle;
		}
		break;
	}

	case EntranceSequenceState::FadingTitle:
	{
		entranceSequenceTimer_ += deltaTime;
		const float fadeDuration = titleFadeDuration_ > 0.001f ? titleFadeDuration_ : 0.001f;
		const float progress = std::clamp(entranceSequenceTimer_ / fadeDuration, 0.0f, 1.0f);
		const float titleAlpha = SmoothStep01(progress);
		sprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, titleAlpha });

		if (progress >= 1.0f)
		{
			sprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			entranceSequenceTimer_ = 0.0f;
			entranceSequenceState_ = EntranceSequenceState::Complete;
		}
		break;
	}

	case EntranceSequenceState::Complete:
		break;
	}
}

void TitleScene::SkipEntranceSequence()
{
	player_->SetTranslate(entrancePlayerFinalPosition_);
	player_->SetTitleOpacity(1.0f);
	SetTitleCameraTransform(
		entranceCameraFinalPosition_, entranceCameraFinalRotation_);
	openingFadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	sprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	entranceSequenceTimer_ = 0.0f;
	entranceSequenceState_ = EntranceSequenceState::Complete;
}

void TitleScene::SetTitleCameraTransform(const Vector3& translate, const Vector3& rotate)
{
	defaultCameraController_->GetWorldTransform()->translate_ = translate;
	defaultCameraController_->GetWorldTransform()->SetRotate(rotate);
	camera_->SetTranslate(translate);
	camera_->SetRotate(rotate);
}

void TitleScene::UpdateExitSequence(float deltaTime)
{
	switch (exitSequenceState_)
	{
	case ExitSequenceState::WaitingForInput:
		if (entranceSequenceState_ == EntranceSequenceState::Complete &&
			inputManager_->TriggerKey(DIK_SPACE) &&
			!SceneManager::GetInstance()->GetSceneTransition().IsRunning())
		{
			exitStartPlayerPosition_ = player_->GetTranslate();
			exitSequenceTimer_ = 0.0f;
			exitSequenceState_ = ExitSequenceState::FadingTitle;
		}
		break;

	case ExitSequenceState::FadingTitle:
	{
		exitSequenceTimer_ += deltaTime;
		const float fadeDuration =
			titleFadeOutDuration_ > 0.001f ? titleFadeOutDuration_ : 0.001f;
		const float progress = std::clamp(exitSequenceTimer_ / fadeDuration, 0.0f, 1.0f);
		const float titleAlpha = 1.0f - SmoothStep01(progress);
		sprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, titleAlpha });

		if (progress >= 1.0f)
		{
			sprites_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
			ringEmitter_->SetTranslate(player_->GetWorldPosition());
			ringEmitter_->Emit();
			StartExitBlur();
			exitSequenceTimer_ = 0.0f;
			exitSequenceState_ = ExitSequenceState::MovingPlayer;
		}
		break;
	}

	case ExitSequenceState::MovingPlayer:
	{
		exitSequenceTimer_ += deltaTime;
		const float moveDuration = exitMoveDuration_ > 0.001f ? exitMoveDuration_ : 0.001f;
		const float progress = std::clamp(exitSequenceTimer_ / moveDuration, 0.0f, 1.0f);
		// 落下が加速して見えるように、移動量へEase-Inを適用する。
		const float easedProgress = progress * progress;
		UpdateExitBlur(easedProgress);

		Vector3 playerPosition = exitStartPlayerPosition_;
		playerPosition.y -= exitMoveDistance_ * easedProgress;
		player_->SetTranslate(playerPosition);

		if (progress >= 1.0f)
		{
			exitSequenceTimer_ = 0.0f;
			exitSequenceState_ = ExitSequenceState::WaitingForDissolve;
		}
		break;
	}

	case ExitSequenceState::WaitingForDissolve:
	{
		exitSequenceTimer_ += deltaTime;
		const float dissolveDelay = dissolveDelay_ > 0.0f ? dissolveDelay_ : 0.0f;
		if (exitSequenceTimer_ >= dissolveDelay &&
			!SceneManager::GetInstance()->GetSceneTransition().IsRunning())
		{
			SceneManager::GetInstance()->SetSceneRequest("GAMEPLAY");
			exitSequenceState_ = ExitSequenceState::Dissolving;
		}
		break;
	}

	case ExitSequenceState::Dissolving:
		break;
	}
}

void TitleScene::StartExitBlur()
{
	auto* controller = SceneManager::GetInstance()->GetPostEffectController();
	if (controller->IsActive(exitBlurHandle_))
	{
		return;
	}

	exitBlurHandle_ = controller->Emit<RadialBlur>(
		"RadialBlur", std::nullopt, nullptr);
	controller->Edit<RadialBlur>(
		exitBlurHandle_,
		[](RadialBlur& blur)
		{
			blur.SetCenter({ 0.5f, 0.5f });
			blur.SetBlurWidth(0.0f);
		});
}

void TitleScene::UpdateExitBlur(float progress)
{
	auto* controller = SceneManager::GetInstance()->GetPostEffectController();
	if (!controller->IsActive(exitBlurHandle_))
	{
		return;
	}

	const float blurWidth = exitBlurMaxWidth_ * std::clamp(progress, 0.0f, 1.0f);
	controller->Edit<RadialBlur>(
		exitBlurHandle_,
		[blurWidth](RadialBlur& blur)
		{
			blur.SetBlurWidth(blurWidth);
		});
}

void TitleScene::StopExitBlur()
{
	if (exitBlurHandle_ == kInvalidPostEffectHandle)
	{
		return;
	}

	SceneManager::GetInstance()->GetPostEffectController()->Cancel(exitBlurHandle_);
	exitBlurHandle_ = kInvalidPostEffectHandle;
}

void TitleScene::FinishFadeIn()
{
	
}

void TitleScene::Draw()
{
	skyBox_->Draw();
	caveFogEndPlane_->Draw();
	terrain_->Draw();

	if (isDrawObject3d_)
	{
		object3dManager_->DrawingCommon();

		for (size_t i = 0; i < object3ds_.size(); i++)
		{
			object3ds_[i]->Draw();
		}

	}

	player_->Draw();
	particleManager_->Draw();

	/*for (size_t i = 0; i < particle3ds.size(); i++)
	{
		particle3ds[i]->Draw();
	}*/


	if (isDrawSprite_ &&
		(entranceSequenceState_ == EntranceSequenceState::FadingTitle ||
		 entranceSequenceState_ == EntranceSequenceState::Complete))
	{
		for (size_t i = 0; i < sprites_.size(); i++)
		{
			sprites_[i]->Draw();
		}
	}

	if (entranceSequenceState_ == EntranceSequenceState::Complete &&
		exitSequenceState_ == ExitSequenceState::WaitingForInput)
	{
		spacePromptText_->Draw();
	}

	if (openingFadeSprite_->GetColor().w > 0.0f)
	{
		openingFadeSprite_->Draw();
	}

	if (inputManager_->TriggerKey(DIK_0))
	{
		SoundManager::GetInstance()->SoundPlayWave("Alarm01.wav");
	}
	if (inputManager_->TriggerKey(DIK_1))
	{
		SoundManager::GetInstance()->SoundPlayWave("test.mp3");

	}

	cameraManager_->DrawDebugUI();

#ifdef _DEBUG
	debugManager_->DrawAll(cameraManager_->GetMainCamera()->GetViewProjectionMatrix());
#endif

}

