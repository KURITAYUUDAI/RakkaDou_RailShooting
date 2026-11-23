#pragma once
#include "Animation/AnimationUtility.h"
#include "Animation/Skeleton.h"
#include "Animation/MeshSkin.h"

#include <optional>

// Displays the transform and clip-space state of a Skeleton joint connection.
class SkeletonImGuiDebug
{
public:
	void Draw(
		const SkeletonPose& skeleton,
		const Matrix4x4& objectWorldMatrix,
		const Matrix4x4& viewProjectionMatrix);

private:
	struct ClipState
	{
		Vector4 position{};
		bool isFinite = false;
		bool isBehindCamera = false;
		bool isOutsideFrustum = false;
	};

	void DrawJointTree(const SkeletonPose& skeleton, int32_t jointIndex);
	void DrawSelectedJoint(
		const SkeletonPose& skeleton,
		const Matrix4x4& objectWorldMatrix,
		const Matrix4x4& viewProjectionMatrix) const;

	static Vector4 TransformToClip(const Vector3& position, const Matrix4x4& viewProjectionMatrix);
	static ClipState MakeClipState(const Vector3& worldPosition, const Matrix4x4& viewProjectionMatrix);
	static bool IsLineOutsideFrustum(const ClipState& start, const ClipState& end);

	std::optional<int32_t> selectedJointIndex_;
};
