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
#include "Skeleton.h"



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

SkinCluster CreateSkinCluster(const SkeletonAsset& skeletonAsset,
	const MeshGeometry& mesh, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap);

void UpdateSkinCluster(SkinCluster& skincluster, const SkeletonPose& skeletonPose);
