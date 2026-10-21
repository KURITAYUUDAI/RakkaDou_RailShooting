#pragma once
#include "DirectXBase.h"
#include <memory>
#include <queue>

// SRV管理
class SrvManager
{
public:
	// namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:


	// 初期化
	void Initialize(DirectXBase* dxBase);

	void PreDraw();

	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

	void SetComputeRootDescriptorTable(UINT rootParameterIndex, uint32_t uavIndex);

	uint32_t AllocateIndex() 
	{
		if (!freeIndices_.empty()) {
			uint32_t index = freeIndices_.front();
			freeIndices_.pop();
			return index; // 空きがあれば再利用
		}
		return currentSRVCount_++; // なければ新規発番
	}

	void FreeIndex(uint32_t index) {
		freeIndices_.push(index); // 使い終わったらプールに戻す
	}

	// SRV生成（テクスチャ用）
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DirectX::TexMetadata metadata);
	// SRV生成（フォント用）
	void CreateSRVforFontAtlas(uint32_t srvIndex, ID3D12Resource* pResource, int mipLevels, int mostDetailedMip);
	// SRV生成（Structured Buffer用）
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, 
		UINT numElements, UINT structureByteStride);
	// UAV生成（Structured Buffer用）
	void CreateUAVforStructuredBuffer(uint32_t uavIndex, ID3D12Resource* pResource, UINT numVertex, UINT structureByteStride);

public: // 外部入出力

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const uint32_t index);

	ID3D12DescriptorHeap* GetDescriptorHeap();

public:

	// 最大SRV数（最大テクスチャ枚数）
	static const uint32_t kMaxSRVCount;

public:

	// シングルトンインスタンスの取得
	static SrvManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class SrvManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit SrvManager(ConstructorKey){}

private:

	// unique_ptr の型定義に Deleter を入れることでdeleteが可能になる
	static std::unique_ptr<SrvManager> instance_;

	~SrvManager() = default;
	SrvManager(SrvManager&) = delete;
	SrvManager& operator=(SrvManager&) = delete;

	friend struct std::default_delete<SrvManager>;


private:
	
	// DirectXBase
	DirectXBase* dxBase_ = nullptr; 

	
	// SRV用のヒープでディスクリプタの数は128、SRVはShader内で触るものなので、ShaderVisibleはtrue
	ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;
	uint32_t descriptorSize_ = 0;

	std::queue<uint32_t> freeIndices_; // 空いている（再利用可能な）インデックスのプール
	uint32_t currentSRVCount_ = 1;        // 新規に発番するときのカウンタ


};

