#pragma once
#include "DirectXBase.h"
#include <memory>
#include <queue>
#include "../3d/ModelUtility.h"

// UAV管理
class UavManager
{
public:
	// namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	// 初期化
	void Initialize(DirectXBase* dxBase);
	void PreDispatch();
	void SetComputeRootDescriptorTable(UINT RootParameterIndex, uint32_t uavIndex);
	uint32_t AllocateUAVIndex()
	{
		if (!freeUAVIndices_.empty()) {
			uint32_t index = freeUAVIndices_.front();
			freeUAVIndices_.pop();
			return index; // 空きがあれば再利用
		}
		return currentUAVCount_++; // なければ新規発番
	}
	void FreeUAVIndex(uint32_t index) {
		freeUAVIndices_.push(index); // 使い終わったらプールに戻す
	}
	// UAV生成（Structured Buffer用）
	void CreateUAVforStructuredBuffer(uint32_t uavIndex, ID3D12Resource* pResource, UINT numVertex);

public: // 外部入出力
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const uint32_t index);
	ID3D12DescriptorHeap* GetDescriptorHeap();

public:
	// 最大UAV数
	static const uint32_t kMaxUAVCount;

public:

	// シングルトンインスタンスの取得
	static UavManager* GetInstance();
	// 終了
	void Finalize();
	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class UavManager;
	};
	// PassKeyを受け取るコンストラクタ
	explicit UavManager(ConstructorKey) {}

private:

	// unique_ptr の型定義に Deleter を入れることでdeleteが可能になる
	static std::unique_ptr<UavManager> instance_;

	~UavManager() = default;
	UavManager(UavManager&) = delete;
	UavManager& operator=(UavManager&) = delete;
	
	friend struct std::default_delete<UavManager>;

private:
	DirectXBase* dxBase_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
	uint32_t descriptorSize_ = 0;
	
	std::queue<uint32_t> freeUAVIndices_;
	uint32_t currentUAVCount_ = 0;

};

