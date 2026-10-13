#include "TitleScene.h"
#include "SceneManager.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "DistanceFogManager.h"

#include "freetype/FreeTypeManager.h"
#include "FontManager.h"
#include "TextManager.h"

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
	camera_->SetRotate({ pi / 2.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 15.0f, 0.0f });


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
	defaultCameraController_->GetWorldTransform()->translate_ = { 0.0f, 20.0f, 0.0f };
	defaultCameraController_->GetWorldTransform()->SetRotate({ pi / 2.0f, 0.0f, 0.0f });

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

	for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<Sprite> newSprite = std::make_unique<Sprite>();
		newSprite->Initialize("uvChecker.png");
		newSprite->SetAnchorPoint(Vector2{ 0.5f, 0.5f });
		sprites_.push_back(std::move(newSprite));
	}

	sprites_[0]->SetTexture("title.png");
	sprites_[0]->SetPosition(Vector2{ 100.0f, 100.0f });
	sprites_[0]->AdjustTextureSize();



	modelManager_->LoadModel("", "plane.obj");
	modelManager_->LoadModel("", "axis.obj");
	modelManager_->LoadModel("", "sphere.obj");

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

	enterSprite_ = std::make_unique<Sprite>();
	enterSprite_->Initialize("ENTER.png");
	enterSprite_->SetPosition({ 20.0f, 540.0f });

	PrimitiveManager::GetInstance()->Initialize();
	PrimitiveManager::RingConfig ringConfig;
	ringConfig.segments = 32;
	ringConfig.innerRadius = 0.0f;
	ringConfig.outerRadius = 1.5f;
	ringConfig.innerColor = { 0.0f, 0.0f, 1.0f, 1.0f };
	ringConfig.outerColor = { 0.0f, 0.5f, 0.0f, 1.0f };
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

	ringEmitter_ = std::make_unique<ParticleEmitter>();
	ringEmitter_->Initialize("ring",
		{ {1.0f, 2.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }, 1, 0.2f);

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

	// シーン初期化終わり

	SoundManager::GetInstance()->SoundLoadFile("", "Alarm01.wav");
	SoundManager::GetInstance()->SoundLoadFile("", "test.mp3");

	FreeTypeManager::GetInstance()->Initialize();
	TextManager::GetInstance()->Initialize();

	titleText_ = std::make_unique<Text>();
	titleText_->InitializeRichText(
		"fonts/x8y12pxTheStrongGamer.ttf",
		32,
		U"TITLE <b><color=#ff4040>SCENE</color></b>\n"
		U"<i><color=#40a0ff>ITALIC</color></i> <u><color=#40ff80>UNDER</color></u> "
		U"<b><i><u><color=#ffd040cc>BOTH</color></u></i></b>");
	titleText_->SetPosition({ 120.0f, 320.0f });


	debugManager_->Initialize();
}

void TitleScene::Finalize()
{
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
	if (inputManager_->TriggerKey(DIK_RETURN))
	{
		// シーン切り替え
		SceneManager::GetInstance()->SetSceneRequest("GAMEPLAY");
		return;
	}

	WorldTransform::AdvanceFrame();

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

	enterSprite_->Update();

	particleManager_->Update(deltaTime);

	//// ImGuiの内部コマンドを生成する
	//ImGui::Render();

	titleText_->Update();
}

void TitleScene::FinishFadeIn()
{
	
}

void TitleScene::Draw()
{
	skyBox_->Draw();
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

	/*particleManager_->Draw();*/

	/*for (size_t i = 0; i < particle3ds.size(); i++)
	{
		particle3ds[i]->Draw();
	}*/


	/*if (isDrawSprite_)
	{
		for (size_t i = 0; i < sprites_.size(); i++)
		{
			sprites_[i]->Draw();
		}
	}*/

	enterSprite_->Draw();

	particleManager_->Draw();

	if (inputManager_->TriggerKey(DIK_0))
	{
		SoundManager::GetInstance()->SoundPlayWave("Alarm01.wav");
	}
	if (inputManager_->TriggerKey(DIK_1))
	{
		SoundManager::GetInstance()->SoundPlayWave("test.mp3");

	}

	/*titleText_->Draw();*/

	cameraManager_->DrawDebugUI();

#ifdef _DEBUG
	debugManager_->DrawAll(cameraManager_->GetMainCamera()->GetViewProjectionMatrix());
#endif

}

