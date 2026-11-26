#define NOMINMAX
#include "CaveGameplayEnvironment.h"

#include "Camera.h"
#include "CameraManager.h"
#include "CaveRailPath.h"
#include "DistanceFogManager.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "ParticleManager.h"
#include "PrimitiveManager.h"
#include "PSOManager.h"
#include "RailCameraController.h"
#include "random/SeedManager.h"
#include "SkyBox.h"
#include "Terrain.h"

#include <algorithm>
#include <cmath>
#include <utility>

CaveGameplayEnvironment::CaveGameplayEnvironment(
	CaveGameplayEnvironmentDependencies dependencies,
	CaveGameplayEnvironmentConfig config)
	: dependencies_(dependencies),
	  config_(std::move(config))
{
}

CaveGameplayEnvironment::~CaveGameplayEnvironment()
{
	Finalize();
}

void CaveGameplayEnvironment::Initialize()
{
	if (initialized_)
	{
		return;
	}

	InitializeWindLines();

	skyBox_ = std::make_unique<SkyBox>();
	skyBox_->Initialize();
	skyBox_->SetCamera(&dependencies_.camera);

	terrain_ = std::make_unique<Terrain>();
	terrain_->Initialize(Terrain::Mode::GameplayCave, &dependencies_.railPath);
	terrain_->SetCaveSpeed(config_.normalRailSpeed);

	InitializeEntryField();
	initialized_ = true;
}

void CaveGameplayEnvironment::Finalize()
{
	if (!initialized_)
	{
		return;
	}

	entrance_.reset();
	ground_.reset();
	fogEndPlane_.reset();
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
	initialized_ = false;
}

void CaveGameplayEnvironment::UpdateWorld(
	float deltaTime, bool updateEntryField)
{
	skyBox_->Update();
	terrain_->Update(
		deltaTime,
		dependencies_.cameraManager.GetCameraWorldPosition(),
		dependencies_.railCamera.GetCurrentDistance(),
		dependencies_.railCamera.GetPathWorldOffset(),
		dependencies_.railCamera.GetRailFrameWorldMatrix());
	UpdateFogEndPlane();

	if (updateEntryField)
	{
		entrance_->Update();
		ground_->Update();
	}
}

void CaveGameplayEnvironment::UpdateFog(float playerHeight)
{
	const float heightAboveEntrance = std::max(playerHeight, 0.0f);
	const float fogEndDistance = config_.fogBaseEnd + heightAboveEntrance;

	auto* distanceFog = DistanceFogManager::GetInstance();
	distanceFog->SetStartDistance(config_.fogBaseStart);
	distanceFog->SetEndDistance(fogEndDistance);
	distanceFog->SetEnabled(true);
	dependencies_.cameraManager.GetMainCamera()->SetFarClip(
		fogEndDistance + 100.0f);
}

void CaveGameplayEnvironment::UpdateWindLines(
	float deltaTime,
	const Vector3& playerWorldPosition,
	float currentSpeed,
	bool active)
{
	if (!active)
	{
		windEmitAccumulator_ = 0.0f;
		return;
	}

	const float speedRatio = std::clamp(
		currentSpeed / config_.normalRailSpeed, 0.0f, 1.0f);
	const float emitRate = config_.windMaxEmitRate *
		(0.15f + 0.85f * speedRatio);
	windEmitAccumulator_ += emitRate * deltaTime;

	auto* random = SeedManager::GetInstance();
	const float minRadius = std::max(0.0f, config_.windMinRadius);
	const float maxRadius = std::max(minRadius, config_.windMaxRadius);
	const float minDepth = std::max(0.0f, config_.windMinDepth);
	const float maxDepth = std::max(minDepth, config_.windMaxDepth);
	const float minLength = std::max(0.1f, config_.windMinLength);
	const float maxLength = std::max(minLength, config_.windMaxLength);
	const float alpha = std::clamp(config_.windAlpha, 0.0f, 1.0f);

	while (windEmitAccumulator_ >= 1.0f)
	{
		windEmitAccumulator_ -= 1.0f;
		const float angle = random->GenerateFloat(0.0f, 2.0f * pi);
		const float radius = random->GenerateFloat(minRadius, maxRadius);
		const float depth = random->GenerateFloat(minDepth, maxDepth);
		const float width = random->GenerateFloat(0.035f, 0.10f);
		const float baseLength = random->GenerateFloat(minLength, maxLength);
		const float length = baseLength * (0.35f + 0.65f * speedRatio);

		EulerTransform transform{};
		transform.scale = { width, length, width };
		transform.translate = {
			playerWorldPosition.x + std::cos(angle) * radius,
			playerWorldPosition.y - depth,
			playerWorldPosition.z + std::sin(angle) * radius,
		};

		const float upwardSpeed = 20.0f + currentSpeed * 0.25f;
		const float lifeTime = random->GenerateFloat(0.35f, 0.70f);
		dependencies_.particleManager.EmitParticle(
			kWindLineGroup, transform,
			{ 0.0f, upwardSpeed, 0.0f },
			{ 0.72f, 0.88f, 1.0f, alpha }, lifeTime);
	}
}

void CaveGameplayEnvironment::Draw(
	bool drawEntryField, bool drawTerrain)
{
	skyBox_->Draw();
	if (!drawTerrain)
	{
		return;
	}

	fogEndPlane_->Draw();
	terrain_->Draw();
	if (drawEntryField)
	{
		ground_->Draw();
		entrance_->Draw();
	}
}

void CaveGameplayEnvironment::SetEntryMode(bool enabled)
{
	terrain_->SetGameplayCaveEntryMode(enabled);
}

Terrain& CaveGameplayEnvironment::GetTerrain()
{
	return *terrain_;
}

SkyBox& CaveGameplayEnvironment::GetSkyBox()
{
	return *skyBox_;
}

uint32_t CaveGameplayEnvironment::GetEnvironmentTextureIndex() const
{
	return skyBox_->GetEnvironmentTextureIndex();
}

void CaveGameplayEnvironment::InitializeWindLines()
{
	PrimitiveManager::GetInstance()->CreateBox("stage_entry_box");
	dependencies_.particleManager.CreateParticleGroup(
		kWindLineGroup, "white1x1.png");
	dependencies_.particleManager.SetModel(
		kWindLineGroup, "stage_entry_box");
	dependencies_.particleManager.SetIsMoveAccelerationField(
		kWindLineGroup, false);
	dependencies_.particleManager.SetIsBillboard(kWindLineGroup, false);
	dependencies_.particleManager.SetBlendMode(
		kWindLineGroup, PSOManager::BlendMode::Normal);
}

void CaveGameplayEnvironment::InitializeEntryField()
{
	ModelManager::GetInstance()->LoadModel("Field", "FieldEntrance.obj");
	ModelManager::GetInstance()->LoadModel("Field", "FieldGround.obj");

	entrance_ = std::make_unique<Object3d>();
	entrance_->Initialize();
	entrance_->SetModel("FieldEntrance.obj");
	entrance_->SetScale(config_.entranceScale);
	entrance_->SetTranslate(config_.entrancePosition);

	ground_ = std::make_unique<Object3d>();
	ground_->Initialize();
	ground_->SetModel("FieldGround.obj");
	ground_->SetScale(config_.groundScale);
	ground_->SetTranslate(config_.groundPosition);

	fogEndPlane_ = std::make_unique<Object3d>();
	fogEndPlane_->Initialize();
	fogEndPlane_->SetModel("plane.obj");
	fogEndPlane_->SetEnableLighting(false);
	fogEndPlane_->SetScale({
		config_.fogEndPlaneScale,
		config_.fogEndPlaneScale,
		1.0f,
	});
	fogEndPlane_->SetColor(DistanceFogManager::GetInstance()->GetColor());
}

void CaveGameplayEnvironment::UpdateFogEndPlane()
{
	auto* distanceFog = DistanceFogManager::GetInstance();
	Camera* camera = dependencies_.cameraManager.GetMainCamera();
	const Vector3 cameraPosition = camera->GetTranslate();
	const float planeDistance = std::max(
		distanceFog->GetEndDistance() - config_.fogEndPlaneOffset,
		0.001f);
	constexpr float kCoverageMargin = 1.05f;
	const float halfHeight =
		std::tan(camera->GetFovY() * 0.5f) * planeDistance * kCoverageMargin;
	const float halfWidth = halfHeight * camera->GetAspectRatio();
	const Quaternion faceCameraRotation = Multiply(
		camera->GetRotateQuat(),
		MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, pi));

	fogEndPlane_->SetScale({
		std::max(config_.fogEndPlaneScale, halfWidth),
		std::max(config_.fogEndPlaneScale, halfHeight),
		1.0f,
	});
	fogEndPlane_->GetWorldTransform()->SetRotateQuat(faceCameraRotation);
	fogEndPlane_->SetTranslate(
		cameraPosition + camera->GetForward() * planeDistance);
	fogEndPlane_->SetColor(distanceFog->GetColor());
	fogEndPlane_->Update();
}
