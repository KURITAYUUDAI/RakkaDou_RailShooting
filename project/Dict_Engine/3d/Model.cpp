#include "Model.h"
//　ファイルやディレクトリに関する操作を行うライブラリ
#include <filesystem>
// ファイルに書いたり読んだりするライブラリ
#include <fstream>
#include "TextureManager.h"
#include "SrvManager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "ResourcePath.h"

void Model::Initialize()
{
	// スプライトの共通処理を受け取る
	modelManager_ = ModelManager::GetInstance();
}

void Model::CreateResources()
{
	CreateVertexResource();
	CreateIndexResource();
	//CreateMaterialResource();
}

void Model::Draw(const UINT& instanceCount)
{
	if (instanceCount <= 0)
	{
		return;
	}

	instanceCount_ = instanceCount;

	ID3D12GraphicsCommandList* commandList = DirectXBase::GetInstance()->GetCommandList();

	for (const auto& mesh : modelData_.meshes)
	{
		commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);	// VBVを設定
		commandList->IASetIndexBuffer(&mesh.indexBufferView);	// IBVを設定

		// 描画！（DrawCall/ドローコール）。
		commandList->DrawIndexedInstanced(UINT(mesh.indices.size()), instanceCount_, 0, 0, 0);

	}
}

void Model::DrawMesh(const uint32_t meshIndex, const UINT& instanceCount, 
	const D3D12_VERTEX_BUFFER_VIEW* additionalVBV, const D3D12_GPU_DESCRIPTOR_HANDLE* additionalGPUHandle)
{
	if (instanceCount <= 0)
	{
		return;
	}

	instanceCount_ = instanceCount;

	ID3D12GraphicsCommandList* commandList = DirectXBase::GetInstance()->GetCommandList();

	const auto& mesh = modelData_.meshes[meshIndex];

	if (additionalVBV)
	{
		D3D12_VERTEX_BUFFER_VIEW vbvs[2] =
		{
			 mesh.vertexBufferView,
			 *additionalVBV
		};
		commandList->IASetVertexBuffers(0, 2, vbvs);	// VBVを設定

		commandList->SetGraphicsRootDescriptorTable(5, *additionalGPUHandle);
	} 
	else
	{
		commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);	// VBVを設定
	}

	commandList->IASetIndexBuffer(&mesh.indexBufferView);	// IBVを設定

	// 描画！（DrawCall/ドローコール）。
	commandList->DrawIndexedInstanced(UINT(mesh.indices.size()), instanceCount_, 0, 0, 0);
}

//Material Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename, const std::string& mtlname)
//{
//	// 1. 中で必要となる変数の宣言
//	Material materialData; // 構築するMaterialData
//	std::string line; // ファイルから読んだ1行を格納するもの
//	bool isLoad = false; // 引数で指定されたマテリアルの行に来た場合読み込む
//
//	// 2. ファイルを開く
//	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
//	assert(file.is_open()); // とりあえず開けなかったら止める
//
//	// 3. 実際にファイルを読み、MaterialDataを構築していく
//	while (std::getline(file, line))
//	{
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier; // 先頭の識別子を読む
//
//		// identifierに応じた処理
//		if (identifier == "newmtl")
//		{
//			std::string materialName;
//			s >> materialName;
//
//			// 指定されたmtlNameと同じならisLaodをtrueにする
//			if (materialName == mtlname)
//			{
//				isLoad = true;
//			} else
//			{
//				isLoad = false;
//			}
//		}
//		if (isLoad)
//		{
//			if (identifier == "map_Kd")
//			{
//				std::string textureFilename;
//				s >> textureFilename;
//				// 連結してファイルパスにする
//				materialData.textureFilePath = directoryPath + "/" + textureFilename;
//			} else if (identifier == "Kd")
//			{
//				s >> materialData.color.x >> materialData.color.y >> materialData.color.z;
//				materialData.color.w = 1.0f;
//			}
//		}
//	}
//
//	// 4. MaterialDataを返す
//
//
//
//	return materialData;
//}

void Model::LoadModelFile(const std::string& directoryPath, const std::string& filename)
{
	Assimp::Importer importer;
	std::string relativePath = directoryPath + "/" + filename;
	std::string fullPath = ResourcePath::MakeString(relativePath);

	const aiScene* scene = importer.ReadFile(fullPath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	modelData_.meshes.clear();
	modelData_.materialAssets.clear();

	// 先にすべてのマテリアルを読み込む
	modelData_.materialAssets.reserve(scene->mNumMaterials);

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
	{
		aiMaterial* aiMaterialData = scene->mMaterials[materialIndex];

		MaterialAsset materialAsset{};

		// マテリアル名
		aiString materialName;

		if (aiMaterialData->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
		{
			materialAsset.name = materialName.C_Str();
		}

		// Materialの初期値
		materialAsset.material.color = { 1.0f, 1.0f, 1.0f, 1.0f };

		materialAsset.material.enableLighting = true;
		materialAsset.material.shininess = 10.0f;
		materialAsset.material.environmentCoefficient = 1.0f;
		materialAsset.material.alphaReference = 0.0f;
		materialAsset.material.uvTransform = MakeIdentity4x4();

		// マテリアルカラー
		aiColor4D materialColor{ 1.0f, 1.0f, 1.0f, 1.0f };

		if (aiMaterialData->Get(AI_MATKEY_BASE_COLOR, materialColor) == AI_SUCCESS ||
			aiMaterialData->Get(AI_MATKEY_COLOR_DIFFUSE, materialColor) == AI_SUCCESS)
		{
			materialAsset.material.color = 
				{ materialColor.r, materialColor.g, materialColor.b, materialColor.a };
		}

		// 使用するテクスチャ種別を決定
		aiTextureType textureType = aiTextureType_NONE;

		if (aiMaterialData->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
		{
			textureType = aiTextureType_BASE_COLOR;
		} 
		else if (aiMaterialData->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			textureType = aiTextureType_DIFFUSE;
		}

		// テクスチャがない場合は白テクスチャ
		std::filesystem::path texturePath = "white1x1.png";

		if (textureType != aiTextureType_NONE)
		{
			aiString assimpTexturePath;

			if (aiMaterialData->GetTexture(
				textureType,
				0,
				&assimpTexturePath) == AI_SUCCESS)
			{
				texturePath =
					std::filesystem::path(directoryPath) /
					std::filesystem::path(assimpTexturePath.C_Str());
			}
		}

		materialAsset.textureFilePath = texturePath.lexically_normal().generic_string();

		TextureManager::GetInstance()->LoadTexture(materialAsset.textureFilePath);

		materialAsset.textureIndex = TextureManager::GetInstance()->
			GetTextureIndexByFilePath(materialAsset.textureFilePath);

		modelData_.materialAssets.push_back(std::move(materialAsset));
	}

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[meshIndex]; 
		assert(mesh->HasNormals()); // 法線がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0)); // TexcoordがないMeshは今回は非対応
		MeshGeometry newMesh;

		newMesh.vertices.resize(mesh->mNumVertices);	// 最初に頂点数分のメモリを確保しておく
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
		{
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
			
			newMesh.vertices[vertexIndex].position = { -position.x, position.y, position.z, 1.0f };
			newMesh.vertices[vertexIndex].normal = { -normal.x, normal.y, normal.z };
			newMesh.vertices[vertexIndex].texcoord = { texcoord.x, texcoord.y };
			newMesh.vertices[vertexIndex].color = {1.0f, 1.0f, 1.0f, 1.0f};
		}

		// Indexの解析
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);	// 三角形のみサポート
			// ここからFaceの中身(Vertex)の解析を行っていく

			for (uint32_t element = 0; element < face.mNumIndices; ++element)
			{
				uint32_t vertexIndex = face.mIndices[element];
				newMesh.indices.push_back(vertexIndex);
			}
		}
		assert(mesh->mMaterialIndex < modelData_.materialAssets.size());

		newMesh.materialIndex = mesh->mMaterialIndex;

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = newMesh.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindsPoseMatrix = MakeAffineMatrix(
				{ scale.x, scale.y, scale.z },
				{ rotate.x, -rotate.y, -rotate.z, rotate.w },
				{ -translate.x, translate.y, translate.z }
			);
			jointWeightData.inverseBindPoseMatrix = Inverse(bindsPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
			{
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId });
			}
		}

		modelData_.meshes.push_back(newMesh);
	}

	

	modelData_.rootNode = ReadNode(scene->mRootNode);
}

void Model::InsertMesh(MeshGeometry mesh)
{
	modelData_.meshes.clear();
	
	modelData_.meshes.push_back(mesh);

	// 手続き生成モデルにも、ファイルから読み込んだモデルと同様に
	// Object3d::Update()で利用できる有効なRootNodeを持たせる。
	modelData_.rootNode.transform.scale = { 1.0f, 1.0f, 1.0f };
	modelData_.rootNode.transform.rotate = { 0.0f, 0.0f, 0.0f, 1.0f };
	modelData_.rootNode.transform.translate = { 0.0f, 0.0f, 0.0f };
	modelData_.rootNode.localMatrix = MakeIdentity4x4();
	modelData_.rootNode.name = "PrimitiveRoot";
	modelData_.rootNode.meshIndices = { 0 };

	MaterialAsset defaultAsset{};

	defaultAsset.name = "Default";
	defaultAsset.material.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	defaultAsset.material.enableLighting = true;
	defaultAsset.material.alphaReference = 0.0f;
	defaultAsset.material.uvTransform = MakeIdentity4x4();
	defaultAsset.textureFilePath = "white1x1.png";

	TextureManager::GetInstance()->LoadTexture(defaultAsset.textureFilePath);

	defaultAsset.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(
			defaultAsset.textureFilePath);

	modelData_.materialAssets.push_back(std::move(defaultAsset));

	CreateVertexResource();


}

Node Model::ReadNode(aiNode* node)
{
	Node result;

	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);	// assimpの行列からSRTを抽出する関数を利用
	result.transform.scale = { scale.x, scale.y, scale.z };	// Scaleはそのまま
	result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };	// x軸を反転、さらに回転方向が逆なので軸を反転させる
	result.transform.translate = { -translate.x, translate.y, translate.z };	// x軸を反転

	//aiMatrix4x4 aiLocalMatrix = node->mTransformation;	// nodeのlocalMatrixを取得
	//aiLocalMatrix.Transpose();	// 列ベクトル形式を行ベクトル形式に転置
	//
	//for (uint32_t i = 0; i < 4; i++)
	//{
	//	for (uint32_t j = 0; j < 4; j++)
	//	{
	//		result.localMatrix.m[i][j] = aiLocalMatrix[i][j];	// 他の要素も同様に
	//	}
	//}

	result.localMatrix = 
		MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str();	// Node名を格納
	result.children.resize(node->mNumChildren);	// 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		// 再帰的に読んで階層構造を作っていく
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}
	return result;
}

void Model::CreateVertexResource()
{
	for (uint32_t meshIndex = 0; meshIndex < modelData_.meshes.size(); ++meshIndex)
	{
		MeshGeometry& mesh = modelData_.meshes[meshIndex];

		// VertexResourceを作成する。
		mesh.vertexResource = modelManager_->GetDxBase()->
			CreateBufferResource(sizeof(VertexData) * modelData_.meshes[meshIndex].vertices.size());

		assert(mesh.vertexResource && "CreateBufferResource failed");

		// VertexBufferViewの設定
		// リソースの先頭のアドレスから使う
		mesh.vertexBufferView.BufferLocation = mesh.vertexResource->GetGPUVirtualAddress();
		// 使用するリソースのサイズは頂点のサイズ
		mesh.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) 
			* modelData_.meshes[meshIndex].vertices.size());
		// 1頂点当たりのサイズ
		mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

		// VertexResourceにデータを書き込むためのアドレスを取得してVertexDataに割り当てる
		mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mesh.vertexData));
		// 頂点データをコピーする
		std::memcpy(mesh.vertexData, modelData_.meshes[meshIndex].vertices.data(), 
			sizeof(VertexData) * modelData_.meshes[meshIndex].vertices.size());
	}
}

void Model::CreateIndexResource()
{
	for (uint32_t meshIndex = 0; meshIndex < modelData_.meshes.size(); ++meshIndex)
	{
		MeshGeometry& mesh = modelData_.meshes[meshIndex];

		// IndexResourceを作成する。
		mesh.indexResource = modelManager_->GetDxBase()->
			CreateBufferResource(sizeof(uint32_t) * modelData_.meshes[meshIndex].indices.size());

		assert(mesh.indexResource && "CreateBufferResource failed");

		// IIndexBufferViewの設定
		// リソースの先頭のアドレスから使う
		mesh.indexBufferView.BufferLocation = mesh.indexResource->GetGPUVirtualAddress();
		// 使用するリソースのサイズは頂点のサイズ
		mesh.indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * modelData_.meshes[meshIndex].indices.size());
		// 1頂点当たりのサイズ
		mesh.indexBufferView.Format = DXGI_FORMAT_R32_UINT;

		// IndexResourceにデータを書き込むためのアドレスを取得してVertexDataに割り当てる
		mesh.indexResource->Map(0, nullptr, reinterpret_cast<void**>(&mesh.indexData));
		// インデックスデータをコピーする
		std::memcpy(mesh.indexData, modelData_.meshes[meshIndex].indices.data(),
			sizeof(uint32_t) * modelData_.meshes[meshIndex].indices.size());
	}
}

//void Model::CreateMaterialResource()
//{
//	for (uint32_t meshIndex = 0; meshIndex < modelData_.meshes.size(); ++meshIndex)
//	{
//		MaterialHelper::CreateMaterialResource()
//
//		// 色の書き込み
//		mesh.materialData_->color = modelData_.meshes[meshIndex].material.color;
//		mesh.materialData_->enableLighting = true;
//		mesh.materialData_->uvTransform = MakeIdentity4x4();
//		mesh.materialData_->shininess = 10.0f;
//		mesh.materialData_->environmentCoefficient = 1.0f;
//		mesh.materialData_->alphaReference = modelData_.meshes[meshIndex].material.alphaReference;
//	}
//}


void Model::CreateSphere()
{
	//isSphere_ = true;

	///// 球の作成

	//const int kSubdivision = 16;
	//int sphereSize = kSubdivision * kSubdivision * 4; // 1つの経度分割で4つの頂点が必要


	//vertexResource_ = 
	//	modelBase_->GetDxBase()->CreateBufferResource(sizeof(VertexData) * sphereSize);

	//
	//// リソースの先頭のアドレスから使う
	//vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//// 使用するリソースのサイズ
	//vertexBufferView_.SizeInBytes = sizeof(VertexData) * kSubdivision * kSubdivision * 4;
	//// 1頂点当たりのサイズ
	//vertexBufferView_.StrideInBytes = sizeof(VertexData);

	//// 書き込むためのアドレスを取得
	//vertexResource_->Map(0, nullptr,
	//	reinterpret_cast<void**>(&vertexData_));

	//// 経度分割1つ分の角度 Φ
	//const float kLonEvery = pi * 2.0f / static_cast<float>(kSubdivision);
	//// 緯度分割1つ分の角度 Θ
	//const float kLatEvery = pi / static_cast<float>(kSubdivision);
	//// 緯度の方向に分割
	//for (int latIndex = 0; latIndex < kSubdivision; ++latIndex)
	//{
	//	float lat = -pi / 2.0f + kLatEvery * latIndex;
	//	// 経度の方向に分割しながら線を描く
	//	for (int lonIndex = 0; lonIndex < kSubdivision; ++lonIndex)
	//	{
	//		uint32_t start = (latIndex * kSubdivision + lonIndex) * 4;
	//		float lon = lonIndex * kLonEvery; // Φ
	//		// 頂点にデータを入力する。基準点a
	//		vertexData_[start].position =
	//		{
	//			std::cos(lat) * std::cos(lon),
	//			std::sin(lat),
	//			std::cos(lat) * std::sin(lon),
	//			1.0f,
	//		};
	//		vertexData_[start].texcoord =
	//		{
	//			static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
	//			1.0f - static_cast<float>(latIndex) / static_cast<float>(kSubdivision),
	//		};
	//		vertexData_[start].normal.x = vertexData_[start].position.x;
	//		vertexData_[start].normal.y = vertexData_[start].position.y;
	//		vertexData_[start].normal.z = vertexData_[start].position.z;

	//		vertexData_[start + 1].position =
	//		{
	//			std::cos(lat + kLatEvery) * std::cos(lon),
	//			std::sin(lat + kLatEvery),
	//			std::cos(lat + kLatEvery) * std::sin(lon),
	//			1.0f,
	//		};
	//		vertexData_[start + 1].texcoord =
	//		{
	//			static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
	//			1.0f - static_cast<float>(latIndex + 1) / static_cast<float>(kSubdivision),
	//		};
	//		vertexData_[start + 1].normal.x = vertexData_[start + 1].position.x;
	//		vertexData_[start + 1].normal.y = vertexData_[start + 1].position.y;
	//		vertexData_[start + 1].normal.z = vertexData_[start + 1].position.z;

	//		vertexData_[start + 2].position =
	//		{
	//			std::cos(lat) * std::cos(lon + kLonEvery),
	//			std::sin(lat),
	//			std::cos(lat) * std::sin(lon + kLonEvery),
	//			1.0f,
	//		};
	//		vertexData_[start + 2].texcoord =
	//		{
	//			static_cast<float>(lonIndex + 1) / static_cast<float>(kSubdivision),
	//			1.0f - static_cast<float>(latIndex) / static_cast<float>(kSubdivision),
	//		};
	//		vertexData_[start + 2].normal.x = vertexData_[start + 2].position.x;
	//		vertexData_[start + 2].normal.y = vertexData_[start + 2].position.y;
	//		vertexData_[start + 2].normal.z = vertexData_[start + 2].position.z;



	//		vertexData_[start + 3].position =
	//		{
	//			std::cos(lat + kLatEvery) * std::cos(lon + kLonEvery),
	//			std::sin(lat + kLatEvery),
	//			std::cos(lat + kLatEvery) * std::sin(lon + kLonEvery),
	//			1.0f,
	//		};
	//		vertexData_[start + 3].texcoord =
	//		{
	//			static_cast<float>(lonIndex + 1) / static_cast<float>(kSubdivision),
	//			1.0f - static_cast<float>(latIndex + 1) / static_cast<float>(kSubdivision),
	//		};
	//		vertexData_[start + 3].normal.x = vertexData_[start + 3].position.x;
	//		vertexData_[start + 3].normal.y = vertexData_[start + 3].position.y;
	//		vertexData_[start + 3].normal.z = vertexData_[start + 3].position.z;
	//	}
	//}

	//// Sphere用のindexResourceを作成する。
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = 
	//	modelBase_->GetDxBase()->CreateBufferResource(sizeof(uint32_t) * kSubdivision * kSubdivision * 6);
	//// IndexBufferViewを作成する
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	//// リソースの先頭のアドレスから使う
	//indexBufferViewSphere.BufferLocation = indexResource->GetGPUVirtualAddress();
	//// 使用するリソースのサイズ
	//indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * kSubdivision * kSubdivision * 6;
	//// 1つのIndexのサイズ
	//indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;
	//// IndexResourceにデータを書き込む
	//uint32_t* indexData = nullptr;
	//// 書き込むためのアドレスを取得
	//indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	//// Indexは6つの三角形で1つの四角形を表すので、6つずつ書き込む
	//for (int latIndex = 0; latIndex < kSubdivision; ++latIndex)
	//{
	//	for (int lonIndex = 0; lonIndex < kSubdivision; ++lonIndex)
	//	{
	//		// Vertexの読み込み開始位置
	//		uint32_t vertexStart = (latIndex * kSubdivision + lonIndex) * 4;
	//		// Indexの書き込み開始位置
	//		uint32_t indexStart = (latIndex * kSubdivision + lonIndex) * 6;

	//		indexData[indexStart + 0] = vertexStart + 0;
	//		indexData[indexStart + 1] = vertexStart + 1;
	//		indexData[indexStart + 2] = vertexStart + 2;
	//		indexData[indexStart + 3] = vertexStart + 1;
	//		indexData[indexStart + 4] = vertexStart + 3;
	//		indexData[indexStart + 5] = vertexStart + 2;
	//	}
	//}



	//// マテリアル用のリソースを作る。
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSphere =
	//	modelBase_->GetDxBase()->CreateBufferResource(sizeof(Material));
	//// マテリアルにデータを書き込む
	//Material* materialDataSphere = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));
	//// 色の書き込み
	//materialDataSphere->color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	//materialDataSphere->enableLighting = true;
	//materialDataSphere->uvTransform = MakeIdentity4x4();

}

