#pragma once
#include "AnimationUtility.h"

struct JointAsset
{
	std::string name;	// 名前
	std::vector<int32_t> children;	// 子Jointのリスト。いなければ空
	int32_t index;	// 自身のIndex
	std::optional<int32_t> parent;	// 親JointのIndex。いなければnull

	QuaternionTransform bindTransform;	// BindPoseのTransform
};

struct JointPose
{
	QuaternionTransform transform;
	Matrix4x4 localMatrix = MakeIdentity4x4();
	Matrix4x4 skeletonSpaceMatrix = MakeIdentity4x4();
};

struct SkeletonAsset
{
	int32_t root;	// RootJointのIndex
	std::map<std::string, int32_t> jointMap;	// Joint名とIndexとの辞書
	std::vector<JointAsset> joints;	// 所属しているジョイント
};

SkeletonAsset CreateSkeletonAsset(const Node& rootNode);

int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<JointAsset>& joints);

class SkeletonPose
{
public:
	void Initialize(const SkeletonAsset* skeletonAsset);

	void ResetToBindPose();

	void ApplyAnimation(const Animation& animation, float animationTime);

	void Update();

	void DrawDebug(const Matrix4x4& worldMatrix);

	void ImGuiDebug();

	const SkeletonAsset* GetSkeletonAsset() const { return skeletonAsset_; }

	const std::vector<JointPose>& GetJoints() const { return joints_; }

	const JointPose& GetJoint(uint32_t index) const { return joints_[index]; }

private:
	const SkeletonAsset* skeletonAsset_ = nullptr;
	std::vector<JointPose> joints_;
};

