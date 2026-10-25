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

AnimationSet LoadAnimationSet(const std::string& directoryPath, const std::string& filename)
{
	AnimationSet animationSet;
	Assimp::Importer importer;

	const std::string relativePath = directoryPath + "/" + filename;
	std::string fullPath = ResourcePath::MakeString(relativePath);
	const aiScene* scene = importer.ReadFile(fullPath.c_str(), 0);
	assert(scene->mNumAnimations != 0);	// アニメーションがない

	for (uint32_t animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
	{
		aiAnimation* animationAssimp = scene->mAnimations[animationIndex];	// 複数アニメーションを格納
		Animation animation;
		
		if (animationAssimp->mName.length > 0) 
		{
			animation.name = animationAssimp->mName.C_Str();
		} 
		else 
		{
			animation.name = "Animation_" + std::to_string(animationIndex);
		}

		double ticksPerSecond = animationAssimp->mTicksPerSecond;

		if (ticksPerSecond == 0.0) {
			ticksPerSecond = 1.0;
		}

		// 時間の単位を秒に変換
		animation.duration = 
			float(animationAssimp->mDuration / ticksPerSecond /*animationAssimp->mTicksPerSecond*/);	

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

		animationSet.animationMap.try_emplace(animation.name, animationIndex);

		animationSet.animations.push_back(std::move(animation));
	}

	return animationSet;
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





