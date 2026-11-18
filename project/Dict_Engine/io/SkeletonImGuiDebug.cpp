#include "SkeletonImGuiDebug.h"

#ifdef USE_IMGUI
#include "ImGuiManager.h"

#include <cmath>
#include <cstdint>

namespace
{
	constexpr ImGuiTreeNodeFlags kLeafFlags =
		ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	bool IsFinite(const Vector3& value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	bool IsFinite(const Vector4& value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) &&
			std::isfinite(value.z) && std::isfinite(value.w);
	}

	void TextVector3(const char* label, const Vector3& value)
	{
		ImGui::Text("%s: (%.5f, %.5f, %.5f)", label, value.x, value.y, value.z);
	}

	void TextVector4(const char* label, const Vector4& value)
	{
		ImGui::Text("%s: (%.5f, %.5f, %.5f, %.5f)",
			label, value.x, value.y, value.z, value.w);
	}
}

void SkeletonImGuiDebug::Draw(
	const SkeletonPose& skeleton,
	const Matrix4x4& objectWorldMatrix,
	const Matrix4x4& viewProjectionMatrix)
{
	ImGui::Begin("Skeleton Debug");
	ImGui::Text("Joint count: %zu", skeleton.GetJoints().size());

	if (skeleton.GetJoints().empty())
	{
		ImGui::End();
		return;
	}

	if (!selectedJointIndex_ || *selectedJointIndex_ < 0 ||
		*selectedJointIndex_ >= static_cast<int32_t>(skeleton.GetJoints().size()))
	{
		selectedJointIndex_ = skeleton.GetSkeletonAsset()->root;
	}

	ImGui::BeginChild("Skeleton Tree", ImVec2(420.0f, 0.0f), true);
	DrawJointTree(skeleton, skeleton.GetSkeletonAsset()->root);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Joint Details", ImVec2(0.0f, 0.0f), true);
	DrawSelectedJoint(skeleton, objectWorldMatrix, viewProjectionMatrix);
	ImGui::EndChild();

	ImGui::End();
}

void SkeletonImGuiDebug::DrawJointTree(const SkeletonPose& skeleton, int32_t jointIndex)
{
	const JointAsset& joint = skeleton.GetSkeletonAsset()->joints[jointIndex];
	const bool isSelected = selectedJointIndex_ && *selectedJointIndex_ == jointIndex;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (isSelected)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (joint.children.empty())
	{
		flags |= kLeafFlags;
	}

	const bool isOpen = ImGui::TreeNodeEx(
		reinterpret_cast<void*>(static_cast<intptr_t>(jointIndex)),
		flags,
		"[%d] %s",
		jointIndex,
		joint.name.c_str());

	if (ImGui::IsItemClicked())
	{
		selectedJointIndex_ = jointIndex;
	}

	if (isOpen && !joint.children.empty())
	{
		for (int32_t childIndex : joint.children)
		{
			DrawJointTree(skeleton, childIndex);
		}
		ImGui::TreePop();
	}
}

void SkeletonImGuiDebug::DrawSelectedJoint(
	const SkeletonPose& skeleton,
	const Matrix4x4& objectWorldMatrix,
	const Matrix4x4& viewProjectionMatrix) const
{
	if (!selectedJointIndex_)
	{
		return;
	}

	const JointAsset& jointAsset = skeleton.GetSkeletonAsset()->joints[*selectedJointIndex_];
	const JointPose& jointPose = skeleton.GetJoint(*selectedJointIndex_);
	const Matrix4x4 jointWorldMatrix = jointPose.skeletonSpaceMatrix * objectWorldMatrix;
	const Vector3 jointWorldPosition = GetWorldPosition(jointWorldMatrix);

	ImGui::Text("Name: %s", jointAsset.name.c_str());
	ImGui::Text("Index: %d", jointAsset.index);
	ImGui::Text("Parent: %s", jointAsset.parent ? skeleton.GetSkeletonAsset()->joints[*jointAsset.parent].name.c_str() : "None");
	ImGui::Text("Children: %zu", jointAsset.children.size());
	ImGui::Separator();

	TextVector3("Local translate", jointPose.transform.translate);
	ImGui::Text("Local rotate: (%.5f, %.5f, %.5f, %.5f)",
		jointPose.transform.rotate.x, jointPose.transform.rotate.y,
		jointPose.transform.rotate.z, jointPose.transform.rotate.w);
	TextVector3("Local scale", jointPose.transform.scale);
	TextVector3("Joint world", jointWorldPosition);

	const ClipState jointClipState = MakeClipState(jointWorldPosition, viewProjectionMatrix);
	TextVector4("Joint clip", jointClipState.position);
	ImGui::Text("Joint finite: %s", jointClipState.isFinite ? "true" : "false");
	ImGui::Text("Joint behind camera: %s", jointClipState.isBehindCamera ? "true" : "false");
	ImGui::Text("Joint outside frustum: %s", jointClipState.isOutsideFrustum ? "true" : "false");

	if (!jointAsset.parent)
	{
		return;
	}

	ImGui::Separator();
	ImGui::Text("Parent-child line state");

	const JointAsset& parentAsset = skeleton.GetSkeletonAsset()->joints[*jointAsset.parent];
	const JointPose& parentPose = skeleton.GetJoint(*jointAsset.parent);
	const Vector3 parentWorldPosition = GetWorldPosition(
		parentPose.skeletonSpaceMatrix * objectWorldMatrix);
	const ClipState parentClipState = MakeClipState(parentWorldPosition, viewProjectionMatrix);

	TextVector3("Parent world", parentWorldPosition);
	TextVector4("Parent clip", parentClipState.position);
	ImGui::Text("Parent finite: %s", parentClipState.isFinite ? "true" : "false");
	ImGui::Text("Parent behind camera: %s", parentClipState.isBehindCamera ? "true" : "false");
	ImGui::Text("Parent outside frustum: %s", parentClipState.isOutsideFrustum ? "true" : "false");

	const bool lineOutsideFrustum = IsLineOutsideFrustum(parentClipState, jointClipState);
	if (!parentClipState.isFinite || !jointClipState.isFinite)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Line result: invalid coordinate");
	}
	else if (lineOutsideFrustum)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Line result: fully outside frustum");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Line result: should be submitted to rasterizer");
	}
}

Vector4 SkeletonImGuiDebug::TransformToClip(
	const Vector3& position,
	const Matrix4x4& viewProjectionMatrix)
{
	return
	{
		position.x * viewProjectionMatrix.m[0][0] + position.y * viewProjectionMatrix.m[1][0] + position.z * viewProjectionMatrix.m[2][0] + viewProjectionMatrix.m[3][0],
		position.x * viewProjectionMatrix.m[0][1] + position.y * viewProjectionMatrix.m[1][1] + position.z * viewProjectionMatrix.m[2][1] + viewProjectionMatrix.m[3][1],
		position.x * viewProjectionMatrix.m[0][2] + position.y * viewProjectionMatrix.m[1][2] + position.z * viewProjectionMatrix.m[2][2] + viewProjectionMatrix.m[3][2],
		position.x * viewProjectionMatrix.m[0][3] + position.y * viewProjectionMatrix.m[1][3] + position.z * viewProjectionMatrix.m[2][3] + viewProjectionMatrix.m[3][3],
	};
}

SkeletonImGuiDebug::ClipState SkeletonImGuiDebug::MakeClipState(
	const Vector3& worldPosition,
	const Matrix4x4& viewProjectionMatrix)
{
	ClipState state{};
	state.position = TransformToClip(worldPosition, viewProjectionMatrix);
	state.isFinite = IsFinite(worldPosition) && IsFinite(state.position);
	state.isBehindCamera = !state.isFinite || state.position.w <= 0.0f;

	if (!state.isBehindCamera)
	{
		const Vector4& p = state.position;
		state.isOutsideFrustum =
			p.x < -p.w || p.x > p.w ||
			p.y < -p.w || p.y > p.w ||
			p.z < 0.0f || p.z > p.w;
	}

	return state;
}

bool SkeletonImGuiDebug::IsLineOutsideFrustum(const ClipState& start, const ClipState& end)
{
	if (!start.isFinite || !end.isFinite)
	{
		return true;
	}

	const Vector4& a = start.position;
	const Vector4& b = end.position;
	return
		(a.x < -a.w && b.x < -b.w) ||
		(a.x > a.w && b.x > b.w) ||
		(a.y < -a.w && b.y < -b.w) ||
		(a.y > a.w && b.y > b.w) ||
		(a.z < 0.0f && b.z < 0.0f) ||
		(a.z > a.w && b.z > b.w);
}
#else
void SkeletonImGuiDebug::Draw(
	const SkeletonPose&,
	const Matrix4x4&,
	const Matrix4x4&)
{
}
#endif
