#include "SkyBox.h"
#include "TextureManager.h"
#include "ModelManager.h" 
#include "Logger.h"
#include "SrvManager.h"
#include "Camera.h"
#include "CameraManager.h"
#include "DistanceFogManager.h"

void SkyBox::Initialize()
{
	// PSOの設定
	PSOManager::PSOConfig config{};
	config.vertexShaderPath = L"resources/shaders/SkyBox/SkyBox.VS.hlsl";
	config.pixelShaderPath = L"resources/shaders/SkyBox/SkyBox.PS.hlsl";

	// RootSignatureの設定
	config.rootSignatureGenerator = [](){
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;
		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();

		staticSamplerDescs.push_back(sampler);
		D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
		descriptorRange[0].BaseShaderRegister = 0; // t0
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters.resize(4);

		// Enum定義 (可読性のため)
		enum {
			kMaterial, kTransform, kTexture, kFog
		};

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	// VertexShaderで使う
		rootParameters[1].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);	// Tableで利用する数

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;	// 距離フォグ b1

		//rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		//rootParameters[3].Descriptor.ShaderRegister = 1;	// レジスタ番号1を使う

		//rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		//rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		//rootParameters[4].Descriptor.ShaderRegister = 2;	// レジスタ番号2を使う





		// シリアライズ
		static D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		descriptionRootSignature.pParameters = rootParameters.data();
		descriptionRootSignature.NumParameters = (UINT)rootParameters.size();
		descriptionRootSignature.pStaticSamplers = staticSamplerDescs.data();
		descriptionRootSignature.NumStaticSamplers = (UINT)staticSamplerDescs.size();


		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr)) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			assert(false);
		}

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		hr = DirectXBase::GetInstance()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		assert(SUCCEEDED(hr));



		return rootSignature;
		};

	config.inputLayoutGenerator = [](){
		return std::vector<D3D12_INPUT_ELEMENT_DESC>
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			//{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		};

	// 深度設定
	config.depthEnable = true;
	config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	config.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	PSOManager::GetInstance()->RegisterPSOConfig(psoName_, config);

	modelData_ = std::make_unique<ModelData>(CreateSkyBox());
	material_ = std::make_unique<MaterialInstance>();
	material_->Initialize();
	material_->SetMaterialAsset(modelData_->materialAssets[0]);

	TextureManager::GetInstance()->LoadTexture("output_skybox.dds");
	material_->SetTexture("output_skybox.dds");
	

	CreateVertexResource();
	CreateIndexResource();
	
	worldTransform_.Initialize();
	worldTransform_.scale_ = { 10000.0f, 10000.0f, 10000.0f };
	worldTransform_.SetRotate({0.0f, 0.0f, 0.0f});
	worldTransform_.translate_ = {0.0f, 0.0f, 0.0f};
}

void SkyBox::Update()
{
	worldTransform_.UpdateMatrix();
	worldTransform_.TransferMatrix(CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix());
}

void SkyBox::Draw()
{
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// wvp用のCBufferの場所を設定
	worldTransform_.SetCBufferTransformationResource(1);
	// VertexBufferViewを設定
	DirectXBase::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &modelData_->meshes[0].vertexBufferView);	// VBVを設定
	// IndexBufferViewを設定
	DirectXBase::GetInstance()->GetCommandList()->IASetIndexBuffer(&modelData_->meshes[0].indexBufferView);
	// マテリアルのCBufferの場所を設定
	material_->Draw(0, 2);
	// 距離フォグ用CBufferの場所を設定
	DistanceFogManager::GetInstance()->SetCBufferResource(3);

	// 描画
	DirectXBase::GetInstance()->GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);

}

void SkyBox::Finalize()
{
	
}

uint32_t SkyBox::GetEnvironmentTextureIndex() const
{
	return material_->GetTextureIndex();
}

void SkyBox::CreateVertexResource()
{
	// VertexResourceを作成する。
	modelData_->meshes[0].vertexResource = DirectXBase::GetInstance()->
		CreateBufferResource(sizeof(VertexData) * modelData_->meshes[0].vertices.size());

	assert(modelData_->meshes[0].vertexResource && "CreateBufferResource failed");

	// VertexBufferViewの設定
	// リソースの先頭のアドレスから使う
	modelData_->meshes[0].vertexBufferView.BufferLocation = modelData_->meshes[0].vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点のサイズ
	modelData_->meshes[0].vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData_->meshes[0].vertices.size());
	// 1頂点当たりのサイズ
	modelData_->meshes[0].vertexBufferView.StrideInBytes = sizeof(VertexData);

	// VertexResourceにデータを書き込むためのアドレスを取得してVertexDataに割り当てる
	modelData_->meshes[0].vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&modelData_->meshes[0].vertexData));
	// 頂点データをコピーする
	std::memcpy(modelData_->meshes[0].vertexData, modelData_->meshes[0].vertices.data(), 
		sizeof(VertexData) * modelData_->meshes[0].vertices.size());
}


void SkyBox::CreateIndexResource()
{
	// IndexBufferResourceを作成する。uint32_t 36個分のサイズを用意する
	modelData_->meshes[0].indexResource = DirectXBase::GetInstance()->CreateBufferResource(sizeof(uint32_t) * 36);
	// IndexBufferResourceにデータを書き込むためのアドレスを取得してindexDataに割り当てる
	modelData_->meshes[0].indexResource->Map(0, nullptr, reinterpret_cast<void**>(&modelData_->meshes[0].indexData));

	// IIndexBufferViewの設定
		// リソースの先頭のアドレスから使う
	modelData_->meshes[0].indexBufferView.BufferLocation = modelData_->meshes[0].indexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点のサイズ
	modelData_->meshes[0].indexBufferView.SizeInBytes = sizeof(uint32_t) * 36;
	// 1頂点当たりのサイズ
	modelData_->meshes[0].indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// IndexResourceにデータを書き込むためのアドレスを取得してVertexDataに割り当てる
	modelData_->meshes[0].indexResource->Map(0, nullptr, reinterpret_cast<void**>(&modelData_->meshes[0].indexData));
	// インデックスデータをコピーする
	std::memcpy(modelData_->meshes[0].indexData, modelData_->meshes[0].indices.data(),
		sizeof(uint32_t) * modelData_->meshes[0].indices.size());
}



ModelData SkyBox::CreateSkyBox()
{
	// 1. 中で必要となる変数の宣言
	ModelData modelData; //メッシュデータ



	std::vector<VertexData> vertexData;
	vertexData.resize(24);

	

	// CubeのVaretxを作っておく(xが横、yが縦、zが奥行)
	std::vector<Vector4> cubeVertex =
	{
		//　正面から見て右側、XZ平面
		{  1.0f,  1.0f,  1.0f, 1.0f }, // 0
		{  1.0f,  1.0f, -1.0f, 1.0f }, // 1
		{  1.0f, -1.0f,  1.0f, 1.0f }, // 2
		{  1.0f, -1.0f, -1.0f, 1.0f }, // 3

		// 正面から見て左側、XZ平面
		{ -1.0f,  1.0f, -1.0f, 1.0f }, // 4
		{ -1.0f,  1.0f,  1.0f, 1.0f }, // 5
		{ -1.0f, -1.0f, -1.0f, 1.0f }, // 6
		{ -1.0f, -1.0f,  1.0f, 1.0f }, // 7
	};

	// 右面 (+X)
	vertexData[0].position = { 1,  1,  1, 1 };  // 右上奥
	vertexData[1].position = { 1,  1, -1, 1 };  // 右上手前
	vertexData[2].position = { 1, -1,  1, 1 };  // 右下奥
	vertexData[3].position = { 1, -1, -1, 1 };  // 右下手前

	// 左面 (-X)
	vertexData[4].position = { -1,  1, -1, 1 };  // 左上手前
	vertexData[5].position = { -1,  1,  1, 1 };  // 左上奥
	vertexData[6].position = { -1, -1, -1, 1 };  // 左下手前
	vertexData[7].position = { -1, -1,  1, 1 };  // 左下奥

	// 奥面 (+Z)
	vertexData[8].position = { -1,  1,  1, 1 };  // 左上
	vertexData[9].position = { 1,  1,  1, 1 };  // 右上
	vertexData[10].position = { -1, -1,  1, 1 };  // 左下
	vertexData[11].position = { 1, -1,  1, 1 };  // 右下

	// 手前面 (-Z)
	vertexData[12].position = { 1,  1, -1, 1 };  // 右上
	vertexData[13].position = { -1,  1, -1, 1 };  // 左上
	vertexData[14].position = { 1, -1, -1, 1 };  // 右下
	vertexData[15].position = { -1, -1, -1, 1 };  // 左下

	// 上面 (+Y)
	vertexData[16].position = { -1,  1, -1, 1 };  // 左手前
	vertexData[17].position = { 1,  1, -1, 1 };  // 右手前
	vertexData[18].position = { -1,  1,  1, 1 };  // 左奥
	vertexData[19].position = { 1,  1,  1, 1 };  // 右奥

	// 下面 (-Y)
	vertexData[20].position = { -1, -1,  1, 1 };  // 左奥
	vertexData[21].position = { 1, -1,  1, 1 };  // 右奥
	vertexData[22].position = { -1, -1, -1, 1 };  // 左手前
	vertexData[23].position = { 1, -1, -1, 1 };  // 右手前


	for (int i = 0; i < 24; i++)
	{
		vertexData[i].texcoord = { 0.0f, 0.0f };
		vertexData[i].normal = { 0.0f, 0.0f, 0.0f };
	}

	std::vector<uint32_t> indexData;
	indexData.resize(36);
	// インデックスデータの値を書き込む
	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 2; indexData[4] = 1; indexData[5] = 3;

	indexData[6] = 4; indexData[7] = 5; indexData[8] = 6;
	indexData[9] = 6; indexData[10] = 5; indexData[11] = 7;

	indexData[12] = 8; indexData[13] = 9; indexData[14] = 10;
	indexData[15] = 10; indexData[16] = 9; indexData[17] = 11;

	indexData[18] = 12; indexData[19] = 13; indexData[20] = 14;
	indexData[21] = 14; indexData[22] = 13; indexData[23] = 15;

	indexData[24] = 16; indexData[25] = 17; indexData[26] = 18;
	indexData[27] = 18; indexData[28] = 17; indexData[29] = 19;

	indexData[30] = 20; indexData[31] = 21; indexData[32] = 22;
	indexData[33] = 22; indexData[34] = 21; indexData[35] = 23;

	MeshGeometry newMesh;
	newMesh.vertices = vertexData;
	newMesh.indices = indexData;

	modelData.meshes.push_back(newMesh);
	
	Material newMaterial;
	newMaterial.color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	newMaterial.enableLighting = true;
	newMaterial.shininess = 10.0f;
	newMaterial.uvTransform = MakeIdentity4x4();

	MaterialAsset newAsset;
	newAsset.name = "skyBox";
	newAsset.material  = newMaterial;
	newAsset.textureFilePath = "";
	newAsset.textureIndex = 0;

	modelData.materialAssets.push_back(newAsset);

	return modelData;
}
