#include "AnimationMemo.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "ResourcePath.h"

#include "DebugDrawManager.h"

#include "ImGuiManager.h"
#include "SrvManager.h"




SkinCluster CreateSkinCluster(const SkeletonAsset& skeletonAsset,
	const MeshGeometry& mesh, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap)
{
	SkinCluster skinCluster;


	// palette用のResourceを確保
	skinCluster.paletteResource = DirectXBase::GetInstance()->CreateConstantBufferResource(
		sizeof(WellForGPU) * skeletonAsset.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette, skeletonAsset.joints.size() };
	uint32_t paletteSrvIndex = SrvManager::GetInstance()->AllocateIndex();
	skinCluster.paletteSrvHandle.first = SrvManager::GetInstance()->GetCPUDescriptorHandle(paletteSrvIndex);
	skinCluster.paletteSrvHandle.second = SrvManager::GetInstance()->GetGPUDescriptorHandle(paletteSrvIndex);

	// palette用のsrvを作成。StructuredBufferでアクセスできるようにする。
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeletonAsset.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(skinCluster.paletteResource.Get(),
		&paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	// influence用のResourceを確保。頂点毎にinfluence情報を追加出来るようにする
	skinCluster.influenceResource = DirectXBase::GetInstance()->CreateConstantBufferResource(
		sizeof(VertexInfluence) * mesh.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * mesh.vertices.size());	// 0埋め。weightを0にしておく。
	skinCluster.mappedInfluence = { mappedInfluence, mesh.vertices.size() };

	// Influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * mesh.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeletonAsset.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(),
		MakeIdentity4x4);

	for (const auto& jointWeight : mesh.skinClusterData) // meshのSkinClusterの情報を解析
	{
		auto it = skeletonAsset.jointMap.find(jointWeight.first);
		// jointWeight.firstはjoint名なので、skeletonAssetに対象となるjointが含まれているか判断
		{
			if (it == skeletonAsset.jointMap.end())
			{
				continue;
			}

			// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
			skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
			for (const auto& vertexWeight : jointWeight.second.vertexWeights)
			{
				auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];
				// 該当のvertexIndexのinfluence情報を参照しておく
				for (uint32_t index = 0; index < kNumMaxInfluence; ++index) // 空いているところに入れる
					if (currentInfluence.weights[index] == 0.0f)
					{
						currentInfluence.weights[index] = vertexWeight.weight;
						currentInfluence.jointIndices[index] = (*it).second;
						break;
					}
			}
		}
	}

	return skinCluster;
}

void UpdateSkinCluster(SkinCluster& skinCluster, const SkeletonPose& skeletonPose)
{
	for (uint32_t jointIndex = 0; jointIndex < skeletonPose.GetJoints().size(); ++jointIndex)
	{
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster.inverseBindPoseMatrices[jointIndex] * skeletonPose.GetJoint(jointIndex).skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}
