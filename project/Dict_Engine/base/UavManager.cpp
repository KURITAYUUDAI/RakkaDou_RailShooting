#include "UavManager.h"

std::unique_ptr<UavManager> UavManager::instance_ = nullptr;

const uint32_t UavManager::kMaxUAVCount = 512;

UavManager* UavManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<UavManager>(ConstructorKey());
	}
	return instance_.get();
}

void UavManager::Finalize()
{
	instance_.reset();
}

void UavManager::Initialize(DirectXBase* dxBase)
{
	dxBase_ = dxBase;

	// デスクリプタヒープの生成
	descriptorHeap_ = DirectXBase::GetInstance()->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxUAVCount, true);

	descriptorSize_ = DirectXBase::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

void UavManager::PreDispatch()
{
	// DescriptorHeapの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap_.Get() };
	DirectXBase::GetInstance()->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void UavManager::CreateUAVforStructuredBuffer(uint32_t uavIndex, ID3D12Resource * pResource, UINT numVertex)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = numVertex;// 頂点数
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc.Buffer.StructureByteStride = sizeof(VertexData); // 頂点構造体のサイズ

	// 第二引数は今はnullptrにしておく
	DirectXBase::GetInstance()->GetDevice()->CreateUnorderedAccessView(
		pResource, nullptr, &uavDesc, GetCPUDescriptorHandle(uavIndex));
}

D3D12_CPU_DESCRIPTOR_HANDLE UavManager::GetCPUDescriptorHandle(const uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize_ * index);

	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE UavManager::GetGPUDescriptorHandle(const uint32_t index)
{

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize_ * index);

	return handleGPU;
}

ID3D12DescriptorHeap* UavManager::GetDescriptorHeap()
{
	return descriptorHeap_.Get();
}

void UavManager::SetComputeRootDescriptorTable(UINT RootParameterIndex, uint32_t uavIndex)
{
	DirectXBase::GetInstance()->GetCommandList()->SetComputeRootDescriptorTable(RootParameterIndex, GetGPUDescriptorHandle(uavIndex));
}
