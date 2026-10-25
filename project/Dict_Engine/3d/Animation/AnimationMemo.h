#pragma once
#include "DirectXBase.h"
#include "myMath.h"
#include "Quaternion.h"

#include "ModelUtility.h"
#include "SplineCurve.h"

#include <map>
#include <cassert>
#include <optional>
#include <span>

#include "AnimationUtility.h"

struct Joint
{
	QuaternionTransform transform;	// Tranform情報
	Matrix4x4 localMatrix;	// localMatrix
	Matrix4x4 skeletonSpaceMatrix;	// skeletonSpaceでの変換行列
	std::string name;	// 名前
	std::vector<int32_t> children;	// 子Jointのリスト。いなければ空
	int32_t index;	// 自身のIndex
	std::optional<int32_t> parent;	// 親JointのIndex。いなければnull
};

struct Skeleton
{
	int32_t root;	// RootJointのIndex
	std::map<std::string, int32_t> jointMap;	// Joint名とIndexとの辞書
	std::vector<Joint> joints;	// 所属しているジョイント
};

void UpdateSkeleton(Skeleton& skeleton);

Skeleton CreateSkeleton(const Node& rootNode);

int32_t CreateJoint(
	const Node& node,
	const std::optional<int32_t>& parent,
	std::vector<Joint>& joints);

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);

void DrawDebug(Skeleton& skeleton, const Matrix4x4& worldMatrix);

void ImGuiDebug(Skeleton& skeleton);

const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence
{
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU
{
	Matrix4x4 skeletonSpaceMatrix;	// 位置用
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;	// 法線用
};

struct SkinCluster
{
	std::vector<Matrix4x4> inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence> mappedInfluence;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};

SkinCluster CreateSkinCluster(const Skeleton& skeleton,
	const MeshGeometry& mesh, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap);

void UpdateSkinCluster(SkinCluster& skincluster, const Skeleton& skeleton);
