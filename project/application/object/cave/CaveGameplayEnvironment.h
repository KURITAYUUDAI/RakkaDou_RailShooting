#pragma once

#include "myMath.h"

#include <memory>

class Camera;
class CameraManager;
class CaveRailPath;
class Object3d;
class ParticleManager;
class RailCameraController;
class SkyBox;
class Terrain;

struct CaveGameplayEnvironmentDependencies
{
	Camera& camera;
	CameraManager& cameraManager;
	RailCameraController& railCamera;
	CaveRailPath& railPath;
	ParticleManager& particleManager;
};

struct CaveGameplayEnvironmentConfig
{
	Vector3 entranceScale = { 1.0f, 1.0f, 1.0f };
	Vector3 entrancePosition = { 0.0f, 0.0f, 0.0f };
	Vector3 groundScale = { 1.0f, 1.0f, 1.0f };
	Vector3 groundPosition = { 0.0f, 0.0f, 0.0f };

	float normalRailSpeed = 300.0f;
	float fogBaseStart = 20.0f;
	float fogBaseEnd = 500.0f;
	float fogEndPlaneScale = 50.0f;
	float fogEndPlaneOffset = 0.0f;

	float windMinRadius = 6.0f;
	float windMaxRadius = 32.0f;
	float windMinDepth = 30.0f;
	float windMaxDepth = 150.0f;
	float windMinLength = 3.0f;
	float windMaxLength = 22.0f;
	float windMaxEmitRate = 48.0f;
	float windAlpha = 0.28f;
};

class CaveGameplayEnvironment
{
public:
	CaveGameplayEnvironment(
		CaveGameplayEnvironmentDependencies dependencies,
		CaveGameplayEnvironmentConfig config = {});
	~CaveGameplayEnvironment();

	void Initialize();
	void Finalize();

	void UpdateWorld(float deltaTime, bool updateEntryField);
	void UpdateFog(float playerHeight);
	void UpdateWindLines(
		float deltaTime,
		const Vector3& playerWorldPosition,
		float currentSpeed,
		bool active);

	void Draw(bool drawEntryField, bool drawTerrain = true);

	void SetEntryMode(bool enabled);
	Terrain& GetTerrain();
	SkyBox& GetSkyBox();
	uint32_t GetEnvironmentTextureIndex() const;

private:
	void InitializeWindLines();
	void InitializeEntryField();
	void UpdateFogEndPlane();

private:
	static constexpr const char* kWindLineGroup = "stage_entry_wind_line";

	CaveGameplayEnvironmentDependencies dependencies_;
	CaveGameplayEnvironmentConfig config_;
	bool initialized_ = false;
	float windEmitAccumulator_ = 0.0f;

	std::unique_ptr<SkyBox> skyBox_;
	std::unique_ptr<Terrain> terrain_;
	std::unique_ptr<Object3d> entrance_;
	std::unique_ptr<Object3d> ground_;
	std::unique_ptr<Object3d> fogEndPlane_;
};
