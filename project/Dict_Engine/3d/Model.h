#pragma once
#include "DirectXBase.h"
#include "myMath.h"
#include "ModelManager.h"
#include "ModelUtility.h"

#include <assimp/scene.h>

// 3Dモデル
class Model
{
public:

	

	


public:

	void Initialize();

	void CreateResources();

	void Update();

	void Draw(const UINT& instanceCount);

	void DrawMesh(const uint32_t meshIndex, const UINT& instanceCount, 
		const D3D12_VERTEX_BUFFER_VIEW* additionalVBV = nullptr,
		const D3D12_GPU_DESCRIPTOR_HANDLE* additionalGPUHandle = nullptr);

	void DrawMeshWithOverrideVertexBuffer(const uint32_t meshIndex, UINT instanceCount,
		const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView);

	void Finalize();

	static Material LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename, const std::string& mtlname);

	// 球作成
	void CreateSphere();

	// 箱作成
	void CreateBox();

	void LoadModelFile(const std::string& directoryPath, const std::string& filename);

	void InsertMesh(MeshGeometry mesh);

	void SetTexture(const std::string& directoryFilePath, uint32_t meshIndex);

	void ResetTexture(uint32_t meshIndex);

private:

	Node ReadNode(aiNode* node);

	// VertexResourceを作成
	void CreateVertexResource();

	// IndexResourceを作成
	void CreateIndexResource();

	//// MaterialResourceを作成
	//void CreateMaterialResource();

	
public:	// 外部入出力

	// セッター
	void SetModelData(const ModelData& modelData) { modelData_ = modelData; }
	void SetInstanceCount(const UINT& instanceCount) { instanceCount_ = instanceCount; }
	
	

	// ゲッター
	const UINT& GetInstanceCount() const { return instanceCount_; }
	
	
	MeshGeometry& GetMesh(uint32_t meshIndex) { return modelData_.meshes[meshIndex]; }
	const uint32_t GetMeshCount() { return static_cast<uint32_t>(modelData_.meshes.size()); }
	MaterialAsset& GetMaterialAsset(uint32_t materialIndex) { return modelData_.materialAssets[materialIndex]; }
	const uint32_t GetMaterialAssetsCount() { return static_cast<uint32_t>(modelData_.materialAssets.size()); }
	const Node& GetRootNode(uint32_t meshIndex) { return modelData_.rootNode; }

private:

	ModelManager* modelManager_ = nullptr;

	// モデルのデータ
	ModelData modelData_;

	//// バッファリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	//// バッファリソース内のデータを指すポインタ
	//VertexData* vertexData_ = nullptr;
	//// バッファリソースの使い道を補足するバッファビュー
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	//// マテリアル用バッファリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	//// バッファリソース内のデータを指すポインタ
	//Material* materialData_ = nullptr;

	bool isSphere_ = false;

	UINT instanceCount_ = 1;

	std::string materialTextureFilePath_;
	uint32_t materialTextureIndex_ = 0;
};

