#include "SrvManager.h"

std::unique_ptr<SrvManager> SrvManager::instance_ = nullptr;

const uint32_t SrvManager::kMaxSRVCount = 512;

SrvManager* SrvManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<SrvManager>(ConstructorKey());
	}
	return instance_.get();
}

void SrvManager::Finalize()
{
	instance_.reset();
}


void SrvManager::Initialize(DirectXBase* dxBase)
{
	dxBase_ = dxBase;

	// デスクリプタヒープの生成
	descriptorHeap_ = DirectXBase::GetInstance()->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);

	descriptorSize_ = DirectXBase::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//CreateRenderTextureSRV();
}

void SrvManager::PreDraw()
{
	// 描画用のDescriptorHeapの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap_.Get() };
	DirectXBase::GetInstance()->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DirectX::TexMetadata metadata)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (metadata.IsCubemap())
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // キューブマップ
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
		srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	}
	DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforFontAtlas(uint32_t srvIndex, ID3D12Resource* pResource, int mipLevels, int mostDetailedMip)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;

	DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(
		pResource,
		&srvDesc,
		GetCPUDescriptorHandle(srvIndex)
	);
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride)
{
	assert(pResource);
	assert(structureByteStride > 0);

	// 要素数が 0 で渡されたらリソースサイズから自動算出（任意）
	if (numElements == 0) {
		const auto& rd = pResource->GetDesc();
		numElements = static_cast<UINT>(rd.Width / structureByteStride);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBuffer は常に UNKNOWN
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = numElements;					// 構造体の個数
	srvDesc.Buffer.StructureByteStride = structureByteStride;	// 構造体1個のバイト数
	
	DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(
		pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateUAVforStructuredBuffer(uint32_t uavIndex, ID3D12Resource* pResource, UINT numVertex, UINT structureByteStride)
{
	assert(pResource);
	assert(uavIndex < kMaxSRVCount);
	assert(numVertex > 0);
	assert(structureByteStride > 0);

	const D3D12_RESOURCE_DESC resourceDesc =
		pResource->GetDesc();

	assert(
		resourceDesc.Flags &
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = numVertex;// 頂点数
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc.Buffer.StructureByteStride = structureByteStride;

	// 第二引数は今はnullptrにしておく
	DirectXBase::GetInstance()->GetDevice()->CreateUnorderedAccessView(
		pResource, nullptr, &uavDesc, GetCPUDescriptorHandle(uavIndex));
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(const uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize_ * index);

	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(const uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize_ * index);

	return handleGPU;
}

ID3D12DescriptorHeap* SrvManager::GetDescriptorHeap()
{
	return descriptorHeap_.Get();
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex)
{

	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

void SrvManager::SetComputeRootDescriptorTable(UINT rootParameterIndex, uint32_t uavIndex)
{
	assert(uavIndex < kMaxSRVCount);

	DirectXBase::GetInstance()->GetCommandList()->SetComputeRootDescriptorTable(rootParameterIndex, GetGPUDescriptorHandle(uavIndex));
}
