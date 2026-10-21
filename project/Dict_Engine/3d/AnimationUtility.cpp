#include "AnimationUtility.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "ResourcePath.h"

#include "DebugDrawManager.h"

#include "ImGuiManager.h"
#include "SrvManager.h"

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename)
{
	Animation animation;	// 今回作るアニメーション
	Assimp::Importer importer;

	std::string relativePath = directoryPath + "/" + filename;
	std::string fullPath = ResourcePath::MakeString(relativePath);

	const aiScene* scene = importer.ReadFile(fullPath.c_str(), 0);
	assert(scene->mNumAnimations != 0);	// アニメーションがない
	aiAnimation* animationAssimp = scene->mAnimations[0];	// 最初のアニメーションだけ採用。
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);	// 時間の単位を秒に変換

	// assimpでは個々のNodeのAnimationをchannelと読んでいるのでchannelを回してNoadAnimationの情報を取ってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex)
	{
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex)
		{
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);	// ここも秒に変換
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };	// 右手→左手
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex)
		{
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);	// ここも秒に変換
			keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y,
							  -keyAssimp.mValue.z,  keyAssimp.mValue.w };	// 右手→左手
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex)
		{
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);	// ここも秒に変換
			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };	// 右手→左手
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}

	// 解析完了
	return animation;
}

Matrix4x4 PlayAnimation(Node rootNode, Animation& animation, float& animationTime, const float& DeltaTime)
{
	NodeAnimation& rootNodeAnimation = animation.nodeAnimations[rootNode.name];
	Vector3	translate = CalculateValue<Vector3>(rootNodeAnimation.translate, animationTime);
	Quaternion rotate = CalculateValue<Quaternion>(rootNodeAnimation.rotate, animationTime);
	Vector3		scale = CalculateValue<Vector3>(rootNodeAnimation.scale, animationTime);
	Matrix4x4 localMatrix = MakeAffineMatrix(scale, rotate, translate);

	return localMatrix;
}

void UpdateSkeleton(Skeleton& skeleton)
{
	// すべてのJointを更新。親が若いので通常ループで処理可能になっている
	for (Joint& joint : skeleton.joints)
	{
		joint.localMatrix = 
			MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent)	// 親がいれば親の行列を掛ける
		{
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		}
		else	// 親がいないのでlocalMatrixとSkeletonSpaceMatrixは一致する
		{
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}


Skeleton CreateSkeleton(const Node& rootNode)
{
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	// 名前とindexのマッピングを行いアクセスしやすくなる
	for (const Joint& joint : skeleton.joints)
	{
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	UpdateSkeleton(skeleton);

	return skeleton;
}

int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size());	// 現在登録されてる数をIndexに
	joint.parent = parent;
	joints.push_back(joint);	// SkeletonのJoint列に登録
	for (const Node& child : node.children)
	{
		// 子Jointを作成し、そのIndexを登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	// 自身のIndexを返す
	return joint.index;
}

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime)
{
	for (Joint& joint : skeleton.joints)
	{
		// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end())
		{
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = CalculateValue(rootNodeAnimation.translate, animationTime);
			joint.transform.rotate = CalculateValue(rootNodeAnimation.rotate, animationTime);
			joint.transform.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
		}
	}
}

void DrawDebug(Skeleton& skeleton, const Matrix4x4& worldMatrix)
{
	DebugDrawManager* debugDrawManager = DebugDrawManager::GetInstance();

	// すべてのJointを描画。
	for (Joint& joint : skeleton.joints)
	{
		Vector3 worldTranslate;
		worldTranslate = GetWorldPosition(joint.skeletonSpaceMatrix * worldMatrix);

		debugDrawManager->AddSphere(worldTranslate, 0.01f, {1.0f, 1.0f, 1.0f, 1.0f}, 5, 5);

		if (joint.parent)
		{
			Vector3 parentWorldTranslate;
			parentWorldTranslate = GetWorldPosition(
				skeleton.joints[*joint.parent].skeletonSpaceMatrix * worldMatrix);

			debugDrawManager->AddLine(worldTranslate, parentWorldTranslate, { 1.0f, 1.0f, 1.0f, 1.0f });
		} 
		else
		{
			Vector3 skeletonWorldTranslate = GetWorldPosition(worldMatrix);
			debugDrawManager->AddLine(worldTranslate, skeletonWorldTranslate, { 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}
}

void ImGuiDebug(Skeleton& skeleton)
{
	// すべてのJointを描画。
	for (Joint& joint : skeleton.joints)
	{
		
	}
}

SkinCluster CreateSkinCluster(const Skeleton& skeleton, 
	const MeshGeometry& mesh, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap)
{
	SkinCluster skinCluster;


	// palette用のResourceを確保
	skinCluster.paletteResource = DirectXBase::GetInstance()->CreateConstantBufferResource(
		sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = {mappedPalette, skeleton.joints.size()};
	uint32_t paletteSrvIndex = SrvManager::GetInstance()->AllocateSRVIndex();
	skinCluster.paletteSrvHandle.first = SrvManager::GetInstance()->GetCPUDescriptorHandle(paletteSrvIndex);
	skinCluster.paletteSrvHandle.second = SrvManager::GetInstance()->GetGPUDescriptorHandle(paletteSrvIndex);
	
	// palette用のsrvを作成。StructuredBufferでアクセスできるようにする。
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride =  sizeof(WellForGPU);
	DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(skinCluster.paletteResource.Get(),
		&paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	// influence用のResourceを確保。頂点毎にinfluence情報を追加出来るようにする
	skinCluster.influenceResource = DirectXBase::GetInstance()->CreateConstantBufferResource(
		sizeof(VertexInfluence) * mesh.vertices.size());
	VertexInfluence * mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * mesh.vertices.size());	// 0埋め。weightを0にしておく。
	skinCluster.mappedInfluence = { mappedInfluence, mesh.vertices.size() };

	// Influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * mesh.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(),
		MakeIdentity4x4);

	for (const auto& jointWeight : mesh.skinClusterData) // meshのSkinClusterの情報を解析
	{
		auto it = skeleton.jointMap.find(jointWeight.first);	
		// jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
		{
			if (it == skeleton.jointMap.end())
			{
				continue;
			}

			// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
			skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
			for (const auto& vertexWeight : jointWeight.second.vertexWeights)
			{
				auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];	
				// 該当のvertexIndexのinfluence情報を参照しておく
				for(uint32_t index = 0; index < kNumMaxInfluence; ++index) // 空いているところに入れる
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

void UpdateSkinCluster(SkinCluster& skinCluster, const Skeleton& skeleton)
{
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
	{
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

//void DispatchComputeShader()
//{
//	DirectXBase::GetInstance()->GetCommandList()->SetComputeRootSignature()
//}


