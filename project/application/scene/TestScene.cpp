#include "TestScene.h"
#include "SceneManager.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "PrimitiveManager.h"
#include "Dict_Engine/tool/effect/DissolveManager.h"

namespace
{
	constexpr size_t kScalingBoxIndex = 0;
	constexpr size_t kRotatingBoxIndex = 1;
	constexpr size_t kMovingBoxIndex = 2;
}

void TestScene::Initialize()
{
	// Textureを読んで転送する
	textureManager_->LoadTexture("uvChecker.png");
	textureManager_->LoadTexture("monsterBall.png");
	textureManager_->LoadTexture("title.png");

	int textureIndex = 0;
	const char* textureOptions[] = { "Checker", "monsterBall" };

	// シーン初期化始め
	camera_ = std::make_unique<Camera>();
	camera_->Initialize();
	/*camera_->SetRotate({ pi / 3.0f, pi, 0.0f });
	camera_->SetTranslate({ 0.0f, 9.0f, 5.0f });*/
	camera_->SetRotate({ 0.0f, pi, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, 15.0f });


	/*object3dManager_->SetDefaultCamera(camera_.get());
	particleManager_->SetDefaultCamera(camera_.get());*/

	cameraManager_->Initialize();
	cameraManager_->GetMainCamera()->SetFarClip(300.0f);
	cameraManager_->AddCamera("Default", camera_.get());
	cameraManager_->SetActiveCamera("Default");

	defaultCameraController_ = std::make_unique<DefaultCameraController>();
	defaultCameraController_->Initialize();
	defaultCameraController_->GetWorldTransform()->translate_ = camera_->GetTranslate();
	defaultCameraController_->GetWorldTransform()->SetRotate(camera_->GetRotate());

	cameraManager_->AddCameraController("Default", defaultCameraController_.get());
	cameraManager_->SetActiveCameraController("Default");

	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->Initialize();
	skyBox_->SetCamera(camera_.get());

	LightManager::GetInstance()->Initialize();
	LightManager::GetInstance()->SetDirectionalLightColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	LightManager::GetInstance()->SetDirectionalLightDirection(
		Normalize(Vector3{ 0.0f, -1.0f, -1.0f }));
	LightManager::GetInstance()->SetDirectionalLightIntensity(1.0f);

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



	ModelManager::GetInstance()->LoadModel("", "plane.obj");
	ModelManager::GetInstance()->LoadModel("", "axis.obj");
	PrimitiveManager::GetInstance()->Initialize();
	PrimitiveManager::GetInstance()->CreateBox("box_primitive");

	for (size_t i = 0; i < 3; i++)
	{
		std::unique_ptr<Object3d> newObject3d = std::make_unique<Object3d>();
		newObject3d->Initialize();
		newObject3d->SetModel("box_primitive");
		newObject3d->SetEnableLighting(false);
		object3ds_.push_back(std::move(newObject3d));
	}

	object3ds_[kScalingBoxIndex]->SetTranslate({ -4.0f, 0.0f, 0.0f });
	object3ds_[kScalingBoxIndex]->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });

	object3ds_[kRotatingBoxIndex]->SetTranslate({ 0.0f, 0.0f, 0.0f });
	object3ds_[kRotatingBoxIndex]->SetColor({ 0.2f, 1.0f, 0.2f, 1.0f });

	object3ds_[kMovingBoxIndex]->SetTranslate({ 4.0f, 0.0f, 0.0f });
	object3ds_[kMovingBoxIndex]->SetColor({ 0.2f, 0.4f, 1.0f, 1.0f });
	boxAnimationTime_ = 0.0f;

	animationTest_ = std::make_unique<AnimationTest>();
	animationTest_->Initialize();
	animationTest_->SetScale({ 1.0f, 1.0f, 1.0f });
	animationTest_->SetRotate({ 0.0f, 0.0f, 0.0f });
	animationTest_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	animationTest_->SetEnvironmentTextureIndex(skyBox_->GetEnvironmentTextureIndex());

	particleManager_->SetModel("plane.obj");
	particleManager_->CreateParticleGroup("circle", "circle.png");
	particleManager_->SetModel("circle", "plane.obj");
	movingBoxParticleParent_ = particleManager_->RegisterParticleParent();


	AABB  aabb;
	aabb.max = { 1.0f, 1.0f, 1.0f };
	aabb.min = { -1.0f, -1.0f, -1.0f };
	particleManager_->CreateAccelerationField({ 5.0f, 0.0f, 0.0f }, aabb);


	for (size_t i = 0; i < 1; i++)
	{
		std::unique_ptr<ParticleEmitter> emitter = std::make_unique<ParticleEmitter>();
		EulerTransform transform = { { 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } };
		emitter->Initialize("circle", transform, 3, 0.2f);
		emitter->SetParentHandle(movingBoxParticleParent_);
		emitters_.push_back(std::move(emitter));
	}

	// シーン初期化終わり

	SoundManager::GetInstance()->SoundLoadFile("", "Alarm01.wav");
	SoundManager::GetInstance()->SoundLoadFile("", "test.mp3");
}

void TestScene::Finalize()
{
	for (auto it = emitters_.begin(); it != emitters_.end(); ++it)
	{
		std::unique_ptr<ParticleEmitter> emitter = std::move(*it);
		emitter.reset();

	}
	emitters_.clear();
	particleManager_->UnregisterParticleParent(movingBoxParticleParent_);
	movingBoxParticleParent_ = {};
	particleManager_->Reset();

	animationTest_->Finalize();

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

	if (skyBox_)
	{
		skyBox_->Finalize();
		skyBox_.reset();
	}

	camera_->Finalize();
	camera_.reset();

	LightManager::GetInstance()->Finalize();
	cameraManager_->Finalize();
	//PostEffectManager::GetInstance()->Clear();
}

void TestScene::Update(const float& deltaTime)
{
	if (inputManager_->TriggerKey(DIK_F1) &&
		!SceneManager::GetInstance()->GetSceneTransition().IsRunning())
	{
		SceneManager::GetInstance()->SetSceneRequest("TITLE");
		return;
	}

	if (inputManager_->TriggerKey(DIK_RETURN))
	{
		// シーン切り替え
		SceneManager::GetInstance()->SetSceneRequest("TITLE");
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


	ImGui::Begin("DebugBox");

	for (size_t i = 0; i < object3ds_.size(); ++i)
	{
		Vector3 scale = object3ds_[i]->GetScale();
		ImGui::InputFloat3(("scale" + std::to_string(i)).c_str(), &scale.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
		Vector3 rotate = object3ds_[i]->GetRotate();
		ImGui::InputFloat3(("rotate" + std::to_string(i)).c_str(), &rotate.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
		Vector3 translate = object3ds_[i]->GetTranslate();
		ImGui::InputFloat3(("translate" + std::to_string(i)).c_str(), &translate.x, "%.3f", ImGuiInputTextFlags_ReadOnly);

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

	Vector3 cameraPos = cameraManager_->GetCameraWorldPosition();
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

	cameraManager_->SetActiveCamera("Default");
	cameraManager_->Update(deltaTime);
	skyBox_->Update();

	boxAnimationTime_ += deltaTime;
	const float scale = 1.0f + std::sinf(boxAnimationTime_ * 2.0f) * 0.5f;
	object3ds_[kScalingBoxIndex]->SetScale({ scale, scale, scale });
	object3ds_[kRotatingBoxIndex]->SetRotate({ 0.0f, boxAnimationTime_, 0.0f });
	object3ds_[kMovingBoxIndex]->SetTranslate(
		{ 4.0f, std::sinf(boxAnimationTime_ * 2.0f) * 2.0f, 0.0f });


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
	particleManager_->UpdateParticleParent(
		movingBoxParticleParent_,
		object3ds_[kMovingBoxIndex]->GetWorldTransform()->worldMatrix_);

	for (auto it = emitters_.begin(); it != emitters_.end(); ++it)
	{
		ParticleEmitter* emitter = it->get();
		emitter->Update(deltaTime);
	}

	animationTest_->Update(deltaTime);

	particleManager_->Update(deltaTime);

	//// ImGuiの内部コマンドを生成する
	//ImGui::Render();
}

void TestScene::FinishFadeIn()
{}

void TestScene::Draw()
{
	skyBox_->Draw();

	if (isDrawObject3d_)
	{
		for (size_t i = 0; i < object3ds_.size(); i++)
		{
			object3ds_[i]->Draw();
		}

		animationTest_->Draw();
	}


	particleManager_->Draw();

	/*for (size_t i = 0; i < particle3ds.size(); i++)
	{
		particle3ds[i]->Draw();
	}*/


	if (isDrawSprite_)
	{
		for (size_t i = 0; i < sprites_.size(); i++)
		{
			sprites_[i]->Draw();
		}
	}

	if (inputManager_->TriggerKey(DIK_0))
	{
		SoundManager::GetInstance()->SoundPlayWave("Alarm01.wav");
	}
	if (inputManager_->TriggerKey(DIK_1))
	{
		SoundManager::GetInstance()->SoundPlayWave("test.mp3");

	}
}
