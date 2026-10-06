#pragma once
#include "DirectXBase.h"
#include "myMath.h"

#include <memory>

#include "PSOManager.h"

#include "MaterialInstance.h"

class Model;

class Camera;

class SrvManager;

class ParticleManager
{
public:

	// シングルトンインスタンスの取得
	static ParticleManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class ParticleManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit ParticleManager(ConstructorKey){}

private:

	// unique_ptr の型定義に Deleter を入れることでdeleteが可能になる
	static std::unique_ptr<ParticleManager> instance_;

	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator=(ParticleManager&) = delete;

	friend struct std::default_delete<ParticleManager>;

public:

	struct MaterialData
	{
		std::string textureFilePath;
		uint32_t textureIndex = 0;
		Vector4 color;
	};

	struct TransformationMatrix
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	struct ParticleForGPU
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
		Matrix4x4 uvTransform;
	};

	struct DirectionalLight
	{
		Vector4 color;		//!< ライトの色
		Vector3 direction;	//!< ライトの方向
		float intensity;	//!< 輝度
	};

	struct Particle
	{
		enum class MotionType
		{
			Default,
			ChargeAbsorb,
		};

		EulerTransform transform;	// 変換情報
		EulerTransform uvTransform = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};	
								// UV変換情報
		Vector3 velocity;		// 速度
		Vector4 color;			// 色
		float lifeTime;			// 生存時間
		float currentTime;		// 経過時間
		MotionType motionType = MotionType::Default;
		Vector3 startPosition{};
		Vector3 startScale{ 1.0f, 1.0f, 1.0f };
		float angularVelocity = 0.0f;
		float moveSpeed = 0.0f;
		float initialDistance = 1.0f;
	};

	struct Emitter
	{
		EulerTransform transform;
		uint32_t count;
		float frequency;
		float frequencyTime;
	};

	struct AccelerationField
	{
		Vector3 acceleration;
		AABB area;
	};
	
	struct ParticleGroup
	{
		//MaterialData materialData;									// マテリアルデータ
		std::list<Particle> particles;								// パーティクルリスト

		Model* model;												// モデル
		std::unique_ptr<MaterialInstance> material;					// マテリアル

		uint32_t instanceNum;
		uint32_t maxInstanceNum;									// インスタンス数
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;	// インスタンスバッファリソース
		ParticleForGPU* instancingData = nullptr;					// インスタンスデータを書き込むためのポインタ
		uint32_t instancingSrvIndex;								// インスタンス用SRVのインデックス

		bool isMoveAccelerationField = true;						// 加速場の影響を受けるかどうか
		bool isBillboard = true;
		Vector3 attractionTarget{};

		std::string psoName_;										// PSOの名前
		PSOManager::BlendMode blendMode_;							// ブレンドモード
		PSOManager::FillMode fillMode_;								// フィルモード
	};



public:

	// 初期化
	void Initialize(DirectXBase* dxBase);

	void Update(const float& deltaTime);

	void Draw();

	// PSO等を残したままParticleやAccelerationFieldをリセットする
	void Reset();

	void DrawingCommon();

	void CreateParticleGroup(const std::string name, const std::string textureFilePath);

	void RandomEmit(const std::string name, const Vector3& position, uint32_t count);

	void CreateAccelerationField(const Vector3& acceleration, const AABB& area);

	void EmitSlash(const std::string name, const Vector3& position, uint32_t count);

	void Emit(const std::string name, const Vector3& position, uint32_t count);

	// 周囲からtargetへ縮小しながら吸収される正方形パーティクルを生成
	void EmitChargeAbsorb(const std::string& name, const Vector3& target, uint32_t count);

public: // 外部入出力

	// セッター
	//void SetDefaultCamera(Camera* camera){ defaultCamera_ = camera; }
	void SetModel(const std::string& filePath);

	void SetModel(const std::string& name, std::string filePath);
	void SetIsMoveAccelerationField(const std::string& name, bool isMove);
	void SetIsBillboard(const std::string& name, bool isBillboard);
	void SetAttractionTarget(const std::string& name, const Vector3& target);

	// ゲッター
	DirectXBase* GetDxBase() const { return dxBase_; }
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3D12PipelineState* GetGraphicsPipeLineState(){ return graphicsPipeLineState_.Get(); }
	//Camera* GetDefaultCamera() const { return defaultCamera_; }

	const std::list<std::string> GetParticleGroupName();

private:

	/*void CreateRootSignature(ID3DBlob* signatureBlob);

	void CreateGraphicsPipelineState();*/

	

private:

	float deltaTime_ = 0.0f;

	std::string psoName_ = "ParticleDefault";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::Normal;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kSolid;

	DirectXBase* dxBase_ = nullptr;

	SrvManager* srvManager_ = nullptr;

	Model* model_ = nullptr;

	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_;

	// テクスチャグループのコンテナ
	std::unordered_map<std::string, ParticleGroup> particleGroups_;

	// 場のコンテナ
	std::list<AccelerationField> accelerationFields_;

	// カメラのビュープロジェクション
	Matrix4x4 viewProjectionMatrix_;

};

