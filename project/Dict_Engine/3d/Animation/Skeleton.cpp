#include "Skeleton.h"


SkeletonAsset CreateSkeletonAsset(const Node& rootNode)
{
	SkeletonAsset skeleton;
	skeleton.root = CreateJoint(rootNode, std::nullopt, skeleton.joints);

	// 名前とindexのマッピングを行いアクセスしやすくなる
	for (const JointAsset& joint : skeleton.joints)
	{
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	//UpdateSkeleton(skeleton);

	return skeleton;
}

int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<JointAsset>& joints)
{
	JointAsset joint{};
	joint.name = node.name;
	joint.index = int32_t(joints.size());	// 現在登録されてる数をIndexに
	joint.parent = parent;
	joint.bindTransform = node.transform;
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


void SkeletonPose::Initialize(const SkeletonAsset* skeletonAsset)
{
	assert(skeletonAsset != nullptr);

	skeletonAsset_ = skeletonAsset;

	joints_.resize(skeletonAsset_->joints.size());

	ResetToBindPose();
	Update();
}

void SkeletonPose::ResetToBindPose()
{
	assert(skeletonAsset_ != nullptr);

	for (uint32_t jointIndex = 0; jointIndex < joints_.size(); ++jointIndex) {

		joints_[jointIndex].transform = skeletonAsset_->joints[jointIndex].bindTransform;
	}
}

void SkeletonPose::ApplyAnimation(const Animation& animation, float animationTime)
{
	assert(skeletonAsset_ != nullptr);

	for (uint32_t jointIndex = 0; jointIndex < joints_.size(); ++jointIndex) 
	{

		const JointAsset& joint = skeletonAsset_->joints[jointIndex];

		JointPose& pose = joints_[jointIndex];

		// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文
		if (auto animationIt = animation.nodeAnimations.find(joint.name); 
				 animationIt != animation.nodeAnimations.end()) 
		{
			const NodeAnimation& nodeAnimation = animationIt->second;

			if (!nodeAnimation.translate.keyframes.empty())
			{
				pose.transform.translate = CalculateValue(nodeAnimation.translate, animationTime);
			}

			if (!nodeAnimation.rotate.keyframes.empty())
			{
				pose.transform.rotate = CalculateValue(nodeAnimation.rotate, animationTime);
			}

			if (!nodeAnimation.scale.keyframes.empty())
			{
				pose.transform.scale = CalculateValue(nodeAnimation.scale, animationTime);
			}
		}
	}
}

void SkeletonPose::Update()
{
	assert(skeletonAsset_ != nullptr);

	// すべてのJointを更新。親が若いので通常ループで処理可能になっている
	for (uint32_t jointIndex = 0; jointIndex < joints_.size(); ++jointIndex)
	{
		const JointAsset& joint = skeletonAsset_->joints[jointIndex];
		JointPose& jointPose = joints_[jointIndex];

		jointPose.localMatrix = 
			MakeAffineMatrix(jointPose.transform.scale, jointPose.transform.rotate, jointPose.transform.translate);
		if (joint.parent)	// 親がいれば親の行列を掛ける
		{
			jointPose.skeletonSpaceMatrix = jointPose.localMatrix * joints_[*joint.parent].skeletonSpaceMatrix;
		} 
		else	// 親がいないのでlocalMatrixとSkeletonSpaceMatrixは一致する
		{
			jointPose.skeletonSpaceMatrix = jointPose.localMatrix;
		}
	}
}

void SkeletonPose::DrawDebug(const Matrix4x4& worldMatrix)
{
	DebugDrawManager* debugDrawManager = DebugDrawManager::GetInstance();

	// すべてのJointを描画。
	for (uint32_t jointIndex = 0; jointIndex < joints_.size(); ++jointIndex)
	{
		const JointAsset& joint = skeletonAsset_->joints[jointIndex];
		JointPose& jointPose = joints_[jointIndex];

		Vector3 worldTranslate;
		worldTranslate = GetWorldPosition(jointPose.skeletonSpaceMatrix * worldMatrix);

		debugDrawManager->AddSphere(worldTranslate, 0.01f, { 1.0f, 1.0f, 1.0f, 1.0f }, 5, 5);

		if (joint.parent)
		{
			Vector3 parentWorldTranslate;
			parentWorldTranslate = GetWorldPosition(joints_[*joint.parent].skeletonSpaceMatrix * worldMatrix);

			debugDrawManager->AddLine(worldTranslate, parentWorldTranslate, { 1.0f, 1.0f, 1.0f, 1.0f });
		} 
		else
		{
			Vector3 skeletonWorldTranslate = GetWorldPosition(worldMatrix);
			debugDrawManager->AddLine(worldTranslate, skeletonWorldTranslate, { 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}
}

void SkeletonPose::ImGuiDebug()
{
	// すべてのJointを描画。
	for (uint32_t jointIndex = 0; jointIndex < joints_.size(); ++jointIndex)
	{
		const JointAsset& joint = skeletonAsset_->joints[jointIndex];
		const JointPose& jointPose = joints_[jointIndex];
	}
}