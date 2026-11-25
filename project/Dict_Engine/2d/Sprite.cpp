#include "Sprite.h"
#include "TextureManager.h"
#include "SrvManager.h"

void Sprite::Initialize(std::string textureFilePath)
{
	// スプライトの共通処理を受け取る
	spriteManager_ = SpriteManager::GetInstance();

	CreateVertexResource();

	CreateMaterialResource();

	//CreateTransformationMatrixResource();
	worldTransform_.Initialize();

	// テクスチャファイルパスを受け取る
	textureFilePath_ = textureFilePath;
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	AdjustTextureSize();

	psoName_ = SpriteManager::GetInstance()->GetDefaultPsoName();
	blendMode_ = SpriteManager::GetInstance()->GetDefaultBlendmode();
}

void Sprite::Update()
{


	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	if (isFlipX_)
	{
		left = -left;
		right = -right;
	}

	if (isFlipY_)
	{
		top = -top;
		bottom = -bottom;
	}

	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };
	vertexData_[1].position = { left, top, 0.0f, 1.0f };
	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };
	vertexData_[3].position = { right, top, 0.0f, 1.0f };

	// transformに値を代入
	worldTransform_.SetTranslate(Vector3{ position_.x, position_.y, 0.0f });
	worldTransform_.SetRotate(Vector3{ 0.0f, 0.0f, rotation_ });
	worldTransform_.SetScale(Vector3{ size_.x, size_.y, 1.0f });

	worldTransform_.UpdateMatrix();

	// WorldViewProjectionMatrixを作る
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
		0.0f, 0.0f, float(WindowsAPI::kClientWidth), float(WindowsAPI::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

	worldTransform_.TransferMatrix(viewProjectionMatrix);

	const DirectX::TexMetadata& metaData =
		TextureManager::GetInstance()->GetMetaData(textureFilePath_);
	float tex_left = textureLeftTop_.x / metaData.width;
	float tex_right = (textureLeftTop_.x + textureSize_.x) / metaData.width;
	float tex_top = textureLeftTop_.y / metaData.height;
	float tex_bottom = (textureLeftTop_.y + textureSize_.y) / metaData.height;

	// 頂点リソースにデータを書き込む
	vertexData_[0].texcoord = { tex_left, tex_bottom };
	vertexData_[1].texcoord = { tex_left, tex_top };
	vertexData_[2].texcoord = { tex_right, tex_bottom };
	vertexData_[3].texcoord = { tex_right, tex_top };

	uvTransformMatrix_ = MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix_ = Multiply(uvTransformMatrix_, MakeRotateZMatrix(uvTransform_.rotate.z));
	uvTransformMatrix_ = Multiply(uvTransformMatrix_, MakeTranslateMatrix(uvTransform_.translate));
	materialData_->uvTransform = uvTransformMatrix_;

}

void Sprite::Draw()
{
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Spriteの描画。変更が必要なものだけ変更する。
	spriteManager_->GetDxBase()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());	// PS0を設定

	spriteManager_->GetDxBase()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// SpriteのIndexBufferViewを設定
	spriteManager_->GetDxBase()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

	// SpriteのマテリアルのCBufferの場所を設定
	spriteManager_->GetDxBase()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	// TransformationMatrixCBufferの」場所を設定
	worldTransform_.SetCBufferTransformationResource(1);

	// SRV用のDescriptorTableの先頭を設定。2はrootParameter[2]である。
	spriteManager_->GetDxBase()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSRVHandleGPU(textureFilePath_));

	// 描画！(DrawCall)
	spriteManager_->GetDxBase()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void Sprite::Finalize()
{
}

void Sprite::SetTexture(std::string textureFilePath)
{
	// テクスチャファイルパスを受け取る
	textureFilePath_ = textureFilePath;
}

void Sprite::CreateVertexResource()
{
	// VertexResourceを作成する。indexResourceを使うので頂点数は4。
	vertexResource_ = spriteManager_->GetDxBase()->CreateBufferResource(sizeof(VertexData) * 4);

	// IndexResourceを作成する。
	indexResource_ = spriteManager_->GetDxBase()->CreateBufferResource(sizeof(uint32_t) * 6);

	// VertexBufferViewの設定
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点4つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	// 1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// IndexBufferViewの設定
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// VertexResourceにデータを書き込むためのアドレスを取得してVertexDataに割り当てる
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	// 1枚目の三角形
	vertexData_[0].position = { 0.0f, 1.0f, 0.0f, 1.0f };
	vertexData_[0].texcoord = { 0.0f, 1.0f };
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };
	vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[1].texcoord = { 0.0f, 0.0f };
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };
	vertexData_[2].position = { 1.0f, 1.0f, 0.0f, 1.0f };
	vertexData_[2].texcoord = { 1.0f, 1.0f };
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };
	vertexData_[3].position = { 1.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[3].texcoord = { 1.0f, 0.0f };
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

	// IndexResourceにデータを書き込むためのアドレスを取得してIndexDataに割り当てる
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	indexData_[0] = 0;		indexData_[1] = 1;		indexData_[2] = 2;
	indexData_[3] = 1;		indexData_[4] = 3;		indexData_[5] = 2;
}

void Sprite::CreateMaterialResource()
{
	// マテリアルリソースを作成する。
	materialResource_ = spriteManager_->GetDxBase()->CreateBufferResource(sizeof(Material));
	// MaterialResourceにデータを書き込むためのアドレスを取得してMaterialDataに割り当てる
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	
	// マテリアルデータの初期値を書き込む
	materialData_->color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = false;
	materialData_->uvTransform = MakeIdentity4x4();
}

void Sprite::AdjustTextureSize()
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);

	textureSize_.x = static_cast<float>(metadata.width);
	textureSize_.y = static_cast<float>(metadata.height);

	// 画像サイズをテクスチャサイズに合わせる
	size_ = textureSize_;
}
