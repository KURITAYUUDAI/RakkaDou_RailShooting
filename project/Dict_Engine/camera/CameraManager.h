#pragma once
#include "myMath.h"
#include "DirectXBase.h"
#include <unordered_map>
#include "Camera.h"
#include "DebugCamera.h"

class ICameraController;

class CameraManager
{
public:
	// シングルトンインスタンスの取得
	static CameraManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class CameraManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit CameraManager(ConstructorKey){}

private: 	// シングルトンインスタンス

	static std::unique_ptr<CameraManager> instance_;

	~CameraManager() = default;
	CameraManager(CameraManager&) = delete;
	CameraManager& operator=(CameraManager&) = delete;

	friend struct std::default_delete<CameraManager>;

public:

	struct CameraForGPU
	{
		Vector3 worldPosition;
	};

	struct ProjectionInverse
	{
		Matrix4x4 projectionInverse;
	};

	template<typename T>
	T* Get(const std::string& name)
	{
		auto it = cameraControllers_.find(name);
		if (it != cameraControllers_.end())
		{
			// 発見した ICameraController* を、指定された型 T* に安全にキャスト
			// ※ dynamic_cast は、変換できない型（間違ったコントローラー）の場合は nullptr を返します
			return dynamic_cast<T*>(it->second);
		}
		return nullptr; // 見つからなかった場合
	}

public:

	// 初期化
	void Initialize();

	void Update(const float& deltaTime);

	void DrawDebugUI();

	void SetCbufferCameraResource(UINT RootParameterIndex);

	void SetCbufferProjectionInverseResource(UINT RootParameterIndex);

	void AddCameraController(const std::string& name, ICameraController* controller);

	void AddCamera(const std::string& name, Camera* camera);

public: // 外部入出力

	// カメラリソースのGPUアドレス
	D3D12_GPU_VIRTUAL_ADDRESS GetCameraResourceGPUAddress() const { return cameraResource_->GetGPUVirtualAddress(); }
	
	// 現在アクティブなカメラを取得
	const Vector3& GetCameraWorldPosition() const { return cameraData_->worldPosition; }
	
	// カメラをアクティブにする
	void SetActiveCamera(const std::string& name);
	void SetCameraWorldPosition(const Vector3& worldPosition){ cameraData_->worldPosition = worldPosition; }



	ICameraController* GetCameraController(const std::string& name) const;
	ICameraController* GetActiveCameraController() {return activeCameraController_; }
	Camera* GetMainCamera() const 
	{
		if (isDebugCamera_)
		{
			return debugCamera_.get();
		}
		return mainCamera_.get(); 
	}
	
	void SetActiveCameraController(const std::string& name);


private:

	// カメラ用リソースを作成
	void CreateCameraResource();

	// カメラの逆行列用リソースを作成
	void CreateProjectionInverseResource();

private:

	float deltaTime_ = 0.0f;

	std::unique_ptr<Camera> mainCamera_;
	std::unordered_map<std::string, ICameraController*> cameraControllers_;
	ICameraController* activeCameraController_ = nullptr;

	std::unique_ptr<DebugCamera> debugCamera_;
	bool isDebugCamera_ = false;

	std::unordered_map<std::string, Camera*> cameras_;
	Camera* activeCamera_ = nullptr;

	// カメラ用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	CameraForGPU* cameraData_ = nullptr;

	// カメラの逆行列用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> projectionInverseResource_;
	// バッファリソース内のデータを指すポインタ
	ProjectionInverse* projectionInverseData_ = nullptr;
};

