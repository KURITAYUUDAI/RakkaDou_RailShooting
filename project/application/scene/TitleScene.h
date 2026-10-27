#pragma once
#include "BaseScene.h"
#include "Framework.h"

#include "Text.h"
#include "CameraManager.h"
#include "DefaultCameraController.h"

#include "LightManager.h"
#include "DebugDrawManager.h"
#include "PrimitiveManager.h"
#include "SkyBox.h"
#include "Terrain.h"
#include "player/Player.h"
#include "PostEffectController.h"
#include "application/command/input/InputMapping.h"

class TitleScene : public BaseScene
{
public:

	~TitleScene() override = default;

public:

	void Initialize() override;

	void Finalize() override;

	void Update(const float& deltaTime) override;

	void FinishFadeIn() override;

	void Draw() override;

private:
	enum class EntranceSequenceState
	{
		PlayerFalling,
		WaitingForCamera,
		MovingCamera,
		FadingTitle,
		Complete,
	};

	enum class ExitSequenceState
	{
		WaitingForInput,
		FadingTitle,
		MovingPlayer,
		WaitingForDissolve,
		Dissolving,
	};

	void UpdateEntranceSequence(float deltaTime);
	void SkipEntranceSequence();
	void UpdateExitSequence(float deltaTime);
	void SetTitleCameraTransform(const Vector3& translate, const Vector3& rotate);
	void UpdateCaveFogEndPlane();
	void UpdateWindLines(float deltaTime);
	void StartExitBlur();
	void UpdateExitBlur(float progress);
	void StopExitBlur();

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

	// プリミティブマネージャー
	PrimitiveManager* primitiveManager_ = PrimitiveManager::GetInstance();

private:

	bool isDrawSprite_ = true;

	bool isDrawObject3d_ = false;

	// カメラ
	std::unique_ptr<Camera> camera_ = nullptr;

	/*std::unique_ptr<DebugCamera> debugCamera_;*/
	bool isDebugCamera_ = false;

	std::unique_ptr<DefaultCameraController> defaultCameraController_ = nullptr;
	std::unique_ptr<SkyBox> skyBox_ = nullptr;
	std::unique_ptr<Terrain> terrain_ = nullptr;
	std::unique_ptr<Object3d> caveFogEndPlane_ = nullptr;
	float caveFogEndPlaneScale_ = 50.0f;
	float caveFogEndPlaneOffset_ = 0.0f;
	float windLineEmitAccumulator_ = 0.0f;
	static constexpr float kTitleCaveSpeed_ = 300.0f;
	static constexpr const char* kWindLineGroup_ = "title_wind_line";
	static constexpr float kWindLineMinRadius_ = 6.0f;
	static constexpr float kWindLineMaxRadius_ = 32.0f;
	static constexpr float kWindLineMinDepth_ = 30.0f;
	static constexpr float kWindLineMaxDepth_ = 150.0f;
	static constexpr float kWindLineMinLength_ = 3.0f;
	static constexpr float kWindLineMaxLength_ = 22.0f;
	static constexpr float kWindLineEmitRate_ = 48.0f;
	static constexpr float kWindLineAlpha_ = 0.28f;

	Vector2 mousePosition_ = { 0.0f, 0.0f };

	std::unique_ptr<Sprite> openingFadeSprite_;

	std::unique_ptr<Text> keyboardPromptText_;
	std::unique_ptr<Text> controllerPromptText_;

	std::unique_ptr<ParticleEmitter> ringEmitter_;

	std::unique_ptr<ParticleEmitter> cylinderEmitter_;

	std::unique_ptr<Player> player_;

	EntranceSequenceState entranceSequenceState_ = EntranceSequenceState::PlayerFalling;
	float entranceSequenceTimer_ = 0.0f;
	float entranceFallDuration_ = 4.0f;
	float entranceCameraDelay_ = 1.5f;
	float entranceCameraMoveDuration_ = 0.5f;
	float titleFadeDuration_ = 1.5f;
	float openingFadeDuration_ = 0.75f;
	Vector3 entrancePlayerStartPosition_{};
	Vector3 entrancePlayerFinalPosition_{};
	Vector3 entranceCameraStartPosition_ = { 0.0f, 15.0f, -12.0f };
	Vector3 entranceCameraSidePosition_ = { 0.0f, 0.0f, -12.0f };
	Vector3 entranceCameraFinalPosition_ = { 0.0f, 20.0f, 0.0f };
	Vector3 entranceCameraSideRotation_ = { 0.0f, 0.0f, 0.0f };
	Vector3 entranceCameraFinalRotation_ = { 1.57079632679f, 0.0f, 0.0f };
	float entrancePlayerStartHeight_ = 16.0f;

	ExitSequenceState exitSequenceState_ = ExitSequenceState::WaitingForInput;
	Vector3 exitStartPlayerPosition_{};
	float exitSequenceTimer_ = 0.0f;
	float titleFadeOutDuration_ = 0.25f;
	float exitMoveDuration_ = 1.0f;
	float exitMoveDistance_ = 260.0f;
	float dissolveDelay_ = 0.5f;
	float exitBlurMaxWidth_ = 0.015f;
	PostEffectHandle exitBlurHandle_ = kInvalidPostEffectHandle;

};

