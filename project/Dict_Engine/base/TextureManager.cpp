#include "TextureManager.h"
#include "DirectXBase.h"
#include "StringUtility.h"
#include "SrvManager.h"
#include "ResourcePath.h"

std::unique_ptr<TextureManager> TextureManager::instance_ = nullptr;

// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager* TextureManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<TextureManager>(ConstructorKey());
	}

	return instance_.get();
}

void TextureManager::Finalize()
{
	instance_.reset();
}

void TextureManager::Initialize(DirectXBase* dxBase, SrvManager* srvManager)
{
	// SRVの数と同数
	textureDatas_.reserve(SrvManager::kMaxSRVCount);

	dxBase_ = dxBase;
}

void TextureManager::LoadTexture(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	if (textureDatas_.contains(filePath))
	{
		return;
	}

	// テクスチャ枚数上限チェック
	assert(textureDatas_.size() + kSRVIndexTop < SrvManager::kMaxSRVCount);

	// テクスチャファイルを呼んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::string fullPath = ResourcePath::MakeString(filePath);
	std::wstring filePathW = StringUtility::ConvertString(fullPath);
	// DirectX::WIC_FLAGS_FORCE_SRGB : sRGB空間で作られたモノとして読む。
	HRESULT hr;

	OutputDebugStringA(("LoadTexture: " + fullPath + "\n").c_str());
	
	if (filePathW.ends_with(L".dds"))	// .ddsで終わって居たらddsとみなす。より安全な方法はいくらでもあるので余裕があれば対応するといい
	{
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	}
	else
	{
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	assert(SUCCEEDED(hr));

	// ミップマップの作成　（MipMap : 元画像より小さなテクスチャ群）
	DirectX::ScratchImage mipImages{};

	const DirectX::TexMetadata metadata = image.GetMetadata();

	if (DirectX::IsCompressed(metadata.format) ||
		metadata.width <= 1 ||
		metadata.height <= 1)
	{
		mipImages = std::move(image);
	} 
	else
	{
		hr = DirectX::GenerateMipMaps(
			image.GetImages(),
			image.GetImageCount(),
			metadata,
			DirectX::TEX_FILTER_SRGB,
			4,
			mipImages
		);
		assert(SUCCEEDED(hr));
	}
	
	// テクスチャデータを追加して書き込む
	TextureData& textureData = textureDatas_[filePath];

	textureData.metadata = mipImages.GetMetadata();

	assert(textureData.metadata.width > 0);
	assert(textureData.metadata.height > 0);
	assert(textureData.metadata.arraySize > 0);
	assert(mipImages.GetImageCount() > 0);


	textureData.resource = DirectXBase::GetInstance()->CreateTextureResource(textureData.metadata);

	textureData.srvIndex = SrvManager::GetInstance()->AllocateIndex();
	textureData.srvHandleCPU = SrvManager::GetInstance()->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = SrvManager::GetInstance()->GetGPUDescriptorHandle(textureData.srvIndex);


	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = DirectXBase::GetInstance()->UploadTextureData(textureData.resource.Get(), mipImages
		, DirectXBase::GetInstance()->GetDevice(), DirectXBase::GetInstance()->GetCommandList());

	DirectXBase::GetInstance()->PostUploadTexture();
	intermediateResource.Reset();

	// SRV を生成
	SrvManager::GetInstance()->CreateSRVforTexture2D(

		textureData.srvIndex,
		textureData.resource.Get(),
		textureData.metadata
	);
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSRVHandleGPU(const std::string& filePath)
{
	//// 範囲外指定違反チェック
	//assert(textureIndex < textureDatas_.size());
	TextureData& textureData = textureDatas_[filePath];

	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath)
{
	//// 範囲外指定違反チェック
	//assert(textureIndex < textureDatas_.size());

	TextureData& textureData = textureDatas_[filePath];

	return textureData.metadata;
}

const uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	if (textureDatas_.contains(filePath))
	{
		return textureDatas_[filePath].srvIndex;
	}
	else
	{
		LoadTexture(filePath);
		return textureDatas_[filePath].srvIndex;
	}
}


