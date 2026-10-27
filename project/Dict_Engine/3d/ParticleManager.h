#pragma once
#define NOMINMAX
#include "DirectXBase.h"
#include "myMath.h"

#include <memory>
#include <vector>

#include "PSOManager.h"

#include "MaterialInstance.h"

#include <limits>

class Model;
class Camera;

class SrvManager;


struct ParticleParentHandle
{
	static constexpr uint32_t kInvalidIndex = 0xFFFFFFFF;

	uint32_t index = kInvalidIndex;
	uint32_t generation = 0;

	bool IsValid() const
	{
		return index != kInvalidIndex;
	}
};

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
		EulerTransform transform;	// 変換情報
		EulerTransform uvTransform = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};	
								// UV変換情報
		Vector3 velocity;		// 速度
		Vector4 color;			// 色
		float initialAlpha = 1.0f; // 寿命フェード前の不透明度
		float lifeTime;			// 生存時間
		float currentTime;		// 経過時間
		ParticleParentHandle parentHandle;
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
		bool lockToParentOrigin = false;
		bool isScaleOverLife = false;
		bool useFixedLifeTime = false;
		Vector3 startScale = { 1.0f, 1.0f, 1.0f };
		Vector3 endScale = { 1.0f, 1.0f, 1.0f };
		float fixedLifeTime = 1.0f;

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

	ParticleParentHandle RegisterParticleParent();
	void UpdateParticleParent(
		ParticleParentHandle handle, const Matrix4x4& worldMatrix);
	void UnregisterParticleParent(ParticleParentHandle handle);
	const Matrix4x4* ResolveParticleParent(
		ParticleParentHandle handle) const;

	void DrawingCommon();

	void CreateParticleGroup(const std::string name, const std::string textureFilePath);

	void RandomEmit(const std::string name, const Vector3& position, uint32_t count,
		ParticleParentHandle parentHandle = {});

	void CreateAccelerationField(const Vector3& acceleration, const AABB& area);

	void EmitSlash(const std::string name, const Vector3& position, uint32_t count,
		ParticleParentHandle parentHandle = {});

	void Emit(const std::string name, const Vector3& position, uint32_t count,
		ParticleParentHandle parentHandle = {});
	void EmitParticle(
		const std::string& name, const EulerTransform& transform,
		const Vector3& velocity, const Vector4& color, float lifeTime,
		ParticleParentHandle parentHandle = {});

public: // 外部入出力

	// セッター
	//void SetDefaultCamera(Camera* camera){ defaultCamera_ = camera; }
	void SetModel(const std::string& filePath);

	void SetModel(const std::string& name, std::string filePath);
	void SetIsMoveAccelerationField(const std::string& name, bool isMove);
	void SetIsBillboard(const std::string& name, bool isBillboard);
	void SetLockToParentOrigin(const std::string& name, bool enabled);
	void SetScaleOverLife(
		const std::string& name, const Vector3& startScale, const Vector3& endScale);
	void SetParticleLifeTime(const std::string& name, float lifeTime);
	void SetBlendMode(const std::string& name, PSOManager::BlendMode blendMode);

	// ゲッター
	DirectXBase* GetDxBase() const { return dxBase_; }
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3D12PipelineState* GetGraphicsPipeLineState(){ return graphicsPipeLineState_.Get(); }
	//Camera* GetDefaultCamera() const { return defaultCamera_; }

	const std::list<std::string> GetParticleGroupName();

private:
	struct ParticleParentSlot
	{
		Matrix4x4 worldMatrix = MakeIdentity4x4();
		uint32_t generation = 1;
		bool active = false;
	};

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

	// Particleが参照する親のワールド行列と、再利用可能なスロット番号
	std::vector<ParticleParentSlot> particleParentSlots_;
	std::vector<uint32_t> freeParticleParentIndices_;

	// 場のコンテナ
	std::list<AccelerationField> accelerationFields_;

	// カメラのビュープロジェクション
	Matrix4x4 viewProjectionMatrix_;

};

