#include "CameraManager.h"
//#include "Camera.h"
#include "ICameraController.h"
#include "time/DeltaTimeManager.h"

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

		ImGui::End();

	#endif

		activeCameraController_->Update(mainCamera_.get(), deltaTime_);
		// DebugCamera表示中もゲーム用カメラの行列は更新しておく。
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

