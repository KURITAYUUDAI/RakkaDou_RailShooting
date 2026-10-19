#pragma once
#include "DirectXBase.h"
#include "myMath.h"
#include "PSOManager.h"
#include <vector>
#include <memory>

class DebugDrawManager
{
public:
	// シングルトンインスタンスの取得
	static DebugDrawManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class DebugDrawManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit DebugDrawManager(ConstructorKey){}

private: 	// シングルトンインスタンス

	static std::unique_ptr<DebugDrawManager> instance_;

	~DebugDrawManager() = default;
	DebugDrawManager(DebugDrawManager&) = delete;
	DebugDrawManager& operator=(DebugDrawManager&) = delete;

	friend struct std::default_delete<DebugDrawManager>;

public:

    struct DebugVertex 
	{
        Vector4 position;
        Vector4 color;
    };

public:

    void Initialize();

	bool IsCollisionDebugDrawEnabled() const
	{
		return collisionDebugDrawEnabled_;
	}
	void SetCollisionDebugDrawEnabled(bool enabled)
	{
		collisionDebugDrawEnabled_ = enabled;
	}

    // --- 外部から毎フレーム呼ばれるリクエスト関数 ---
    void AddLine(const Vector3& start, const Vector3& end, const Vector4& color);
    void AddBox(const Vector3& center, const Vector3& size, const Vector4& color);
    void AddSphereAxis(const Vector3& center, float radius, const Vector4& color, uint32_t segments = 16);
	void AddBezier(const Vector3& controlPoint0, const Vector3& controlPoint1,
		const Vector3& controlPoint2, const uint32_t segments, const Vector4& color);
	void AddSpline(const std::vector<Vector3>& controlPoints, const Vector4& color,
		uint32_t segments);
	void AddLoopSpline(const std::vector<Vector3>& controlPoints, 
		const Vector4& color, uint32_t segments);
	void AddSphere(const Vector3& center, float radius, const Vector4& color,
		uint32_t lat = 10, uint32_t lon = 10);

    // --- レンダリングパスの後半でエンジンが呼ぶ関数 ---
    void DrawAll(const Matrix4x4& viewProjectionMatrix);

	void RegisterPSO();
	void CreateVertexBufferResource();
	void CreateTransformationResource();

private:

    // 描画用の動的頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    DebugVertex* vertexData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_ = nullptr;
	Matrix4x4* transformationData_ = nullptr;

	std::string psoName_ = "DebugLine";
	PSOManager::BlendMode blendMode_ = PSOManager::BlendMode::None;
	PSOManager::FillMode fillMode_ = PSOManager::FillMode::kWireFrame;

	// 一度に描画できる最大頂点数（必要に応じて調整）
	UINT maxVertices_ = 100000;

    // 1フレーム分のリクエストを貯めるバッファ
    std::vector<DebugVertex> vertices_;

	bool collisionDebugDrawEnabled_ = true;
};

