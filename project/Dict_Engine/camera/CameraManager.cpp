#define NOMINMAX
#include "CameraManager.h"
//#include "Camera.h"
#include "ICameraController.h"
#include "time/DeltaTimeManager.h"

#include <algorithm>
#include <cmath>

std::unique_ptr<CameraManager> CameraManager::instance_ = nullptr;

CameraManager* CameraManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<CameraManager>(ConstructorKey());
	}
	return instance_.get();
}

void CameraManager::Finalize()
{
	if (mainCamera_)
	{
		mainCamera_->Finalize();
	}
	instance_.reset();
}

void CameraManager::Initialize()
{
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize();

#ifdef _DEBUG

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();

#endif

	/*DefaultCameraController* defaultController = new DefaultCameraController();
	defaultController->Initialize();
	AddCameraController("Default", defaultController);
	SetActiveCameraController("Default");*/

	CreateCameraResource();
	CreateProjectionInverseResource();
}

void CameraManager::Update(const float& deltaTime)
{
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::UI);

	if (activeCameraController_ && mainCamera_)
	{
	#ifdef _DEBUG

		ImGui::Begin("CameraSetting");
		ImGui::Checkbox("DebugCamera", &isDebugCamera_);

		Vector3 cameraRotate = activeCameraController_->GetWorldTransform()->GetRotate();
		Vector3 cameraTranslate = activeCameraController_->GetWorldTransform()->translate_;
		if (ImGui::DragFloat3("CameraRotate", &cameraRotate.x, (1.0f / 180.0f) * pi))
		{
			activeCameraController_->GetWorldTransform()->SetRotate(cameraRotate);
		}
		if (ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.1f))
		{
			activeCameraController_->GetWorldTransform()->translate_ = cameraTranslate;
		}
		ImGui::DragFloat(
			"Camera Distance Offset", &cameraDistanceOffset_, 0.1f, -100.0f, 100.0f);

		ImGui::End();

	#endif

		activeCameraController_->Update(mainCamera_.get(), deltaTime_);
		if (std::abs(cameraDistanceOffset_) > 0.0001f)
		{
			const Vector3& rotate = mainCamera_->GetRotate();
			const float cosPitch = std::cos(rotate.x);
			const Vector3 forward = {
				std::sin(rotate.y) * cosPitch,
				-std::sin(rotate.x),
				std::cos(rotate.y) * cosPitch
			};
			mainCamera_->SetTranslate(
				mainCamera_->GetTranslate() - forward * cameraDistanceOffset_);
		}
		if (shakeTimer_ < shakeDuration_)
		{
			shakeTimer_ += deltaTime_;
			const float progress = std::clamp(
				shakeTimer_ / shakeDuration_, 0.0f, 1.0f);
			const float strength = shakeMagnitude_ * (1.0f - progress);
			const float phase = shakeTimer_ * 95.0f;
			Vector3 shakenPosition = mainCamera_->GetTranslate();
			shakenPosition.x += std::sin(phase) * strength;
			shakenPosition.y += std::sin(phase * 1.37f) * strength;
			mainCamera_->SetTranslate(shakenPosition);
		}
		// DebugCamera の表示中も、ゲーム用カメラはプレイヤーの移動範囲の
		// 基準として使うため、毎フレーム行列を更新しておく。
		mainCamera_->Update();

		if (isDebugCamera_ && debugCamera_)
		{
			// デバッグカメラ自体の行列計算を呼び出す
			debugCamera_->Update();

			// GPUに転送する行列や座標を、通常カメラではなく「デバッグカメラ」のものに差し替える
			cameraData_->worldPosition = debugCamera_->GetTranslate();
			projectionInverseData_->projectionInverse = Inverse(debugCamera_->GetProjectionMatrix());
			return;
		}
		else
		{
			// カメラのワールド座標をGPU用構造体に転送
			cameraData_->worldPosition = mainCamera_->GetTranslate();

			// カメラの逆行列をGPU用構造体に転送
			projectionInverseData_->projectionInverse = Inverse(mainCamera_->GetProjectionMatrix());
		}
	}

	//if (activeCamera_)
	//{
	//	// カメラの更新
	//	activeCamera_->Update();
	//	// カメラのワールド座標をGPU用構造体に転送
	//	cameraData_->worldPosition = activeCamera_->GetTranslate();
	//	// カメラの逆行列をGPU用構造体に転送
	//	projectionInverseData_->projectionInverse = Inverse(activeCamera_->GetProjectionMatrix());
	//}
}

void CameraManager::StartShake(float duration, float magnitude)
{
	shakeDuration_ = std::max(duration, 0.0f);
	shakeMagnitude_ = std::max(magnitude, 0.0f);
	shakeTimer_ = 0.0f;
}

void CameraManager::DrawDebugUI()
{
	if (activeCameraController_ && mainCamera_)
	{
		activeCameraController_->DrawDebugUI(mainCamera_.get(), isDebugCamera_);
	}
}

void CameraManager::SetCbufferCameraResource(UINT RootParameterIndex)
{
	// カメラ用のCBufferをバインド
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(RootParameterIndex, cameraResource_->GetGPUVirtualAddress());
}

void CameraManager::SetCbufferProjectionInverseResource(UINT RootParameterIndex)
{
	// カメラの逆行列用のCBufferをバインド
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(RootParameterIndex, projectionInverseResource_->GetGPUVirtualAddress());
}

void CameraManager::AddCameraController(const std::string& name, ICameraController* controller)
{
	cameraControllers_[name] = controller;
	
	if (!activeCameraController_)
	{
		activeCameraController_ = controller;
	}
}

ICameraController* CameraManager::GetCameraController(const std::string& name) const
{
	auto it = cameraControllers_.find(name);
	if (it != cameraControllers_.end())
	{
		return it->second;
	}
	return nullptr;
}

void CameraManager::SetActiveCameraController(const std::string& name)
{
	auto it = cameraControllers_.find(name);
	if (it != cameraControllers_.end())
	{
		activeCameraController_ = it->second;

	#ifdef _DEBUG

		debugCamera_->SetTargetPosition(activeCameraController_->GetWorldTransform());

	#endif
	}
}

void CameraManager::LimitPlayerInFrustum(
	Vector3& playerLocalPos, const Matrix4x4* playerParentWorldMatrix)
{
	if (!mainCamera_)
	{
		return;
	}

	// プレイヤーの親子構造に依存せず、通常カメラのビュー空間で制限する。
	// 表示が DebugCamera でも移動可能範囲は通常時と同一になる。
	Vector3 playerWorldPos = playerLocalPos;
	if (playerParentWorldMatrix)
	{
		playerWorldPos = TransformPosition(playerLocalPos, *playerParentWorldMatrix);
	}
	Vector3 playerViewPos = TransformPosition(playerWorldPos, mainCamera_->GetViewMatrix());

	// プレイヤーの中心がクリップ面へ触れないよう、前後方向にも余白を設ける。
	constexpr float kDepthMargin = 1.0f;
	const float nearLimit = mainCamera_->GetNearClip() + kDepthMargin;
	const float farLimit = mainCamera_->GetFarClip() - kDepthMargin;
	playerViewPos.z = std::clamp(playerViewPos.z, nearLimit, farLimit);
	const float distance = playerViewPos.z;

	// 2. カメラのゲッターから視野角(fovY)とアスペクト比を取得
	float fovY = mainCamera_->GetFovY();
	float aspectRatio = mainCamera_->GetAspectRatio();

	// 3. 現在の深さ（distance）における、視錐台の縦・横の限界サイズ（半分の値）を計算
	// ※ fovY がラジアン単位であることを前提としています。度数法の場合は std::tan(fovY * 0.5f * (pi / 180.0f)) にしてください。
	float frustumHeightHalf = distance * std::tan(fovY * 0.5f);
	float frustumWidthHalf = frustumHeightHalf * aspectRatio;

	// 4. 画面ぴったりだとプレイヤーの中心点しか残らないため、
	//    プレイヤーのサイズ分（マージン）を考慮して内側に制限を設定します
	float marginFactor = 0.85f; // 0.0〜1.0 で調整（小さいほど画面の内側に制限される）
	float xLimit = frustumWidthHalf * marginFactor;
	float yLimit = frustumHeightHalf * marginFactor;

	// 5. 計算した限界値でビュー空間のXとYをクランプ（制限）する
	playerViewPos.x = std::clamp(playerViewPos.x, -xLimit, xLimit);
	playerViewPos.y = std::clamp(playerViewPos.y, -yLimit, yLimit);

	playerWorldPos = TransformPosition(playerViewPos, mainCamera_->GetWorldMatrix());
	if (playerParentWorldMatrix)
	{
		playerLocalPos = TransformPosition(playerWorldPos, Inverse(*playerParentWorldMatrix));
	}
	else
	{
		playerLocalPos = playerWorldPos;
	}
}



void CameraManager::AddCamera(const std::string& name, Camera* camera)
{
	cameras_[name] = camera;

	if (!activeCamera_)
	{
		activeCamera_ = camera;
	}
}

void CameraManager::SetActiveCamera(const std::string& name)
{
	auto it = cameras_.find(name);
	if (it != cameras_.end())
	{
		activeCamera_ = it->second;
	}
}

void CameraManager::CreateCameraResource()
{
	// カメラ用リソースを作成する
	cameraResource_ = DirectXBase::GetInstance()->CreateBufferResource(sizeof(CameraForGPU));
	// CameraResourceにデータを書き込むためのアドレスを取得してCameraDataに割り当てる
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

	// カメラデータの初期値を書き込む
	cameraData_->worldPosition = { 0.0f, 0.0f, 0.0f };

}

void CameraManager::CreateProjectionInverseResource()
{
	// リソース作成
	projectionInverseResource_ = DirectXBase::GetInstance()->CreateConstantBufferResource(sizeof(ProjectionInverse));
	// データを書き込むためのアドレスを取得してDataに割り当てる
	projectionInverseResource_->Map(0, nullptr, reinterpret_cast<void**>(&projectionInverseData_));

	// 初期値を書き込む
	projectionInverseData_->projectionInverse = MakeIdentity4x4();
}

