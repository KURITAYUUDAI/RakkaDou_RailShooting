#pragma once
#include "AnimationUtility.h"
#include "Skeleton.h"

const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence
{
    std::array<float, kNumMaxInfluence> weights;
    std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct MeshSkinBinding
{
    std::vector<Matrix4x4> inverseBindPoseMatrices;
    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;

    uint32_t inputVerticesSrvIndex = 0;
    uint32_t influenceSrvIndex = 0;
    uint32_t vertexCount = 0;
};

MeshSkinBinding CreateMeshSkinBinding(const SkeletonAsset& skeletonAsset, const MeshGeometry& mesh);

struct SkinningInformation
{
    uint32_t numVertices = 0;
};

struct WellForGPU
{
    Matrix4x4 skeletonSpaceMatrix;	// 位置用
    Matrix4x4 skeletonSpaceInverseTransposeMatrix;	// 法線用
};

class MeshSkin
{
public:

    void Initialize(const SkeletonAsset& skeletonAsset, const MeshGeometry& mesh);

    void UpdatePalette(const MeshSkinBinding& binding, const SkeletonPose& skeletonPose);

    void Dispatch(const MeshSkinBinding& binding, const std::string& computePsoName);

    void CreatePalette(uint32_t jointCount);
    void CreateOutputVertexBuffer(const MeshGeometry& mesh);
    void CreateSkinningInfoResource();

public:

    const D3D12_VERTEX_BUFFER_VIEW& GetOutputVertexBufferView() const { return outputVertexBufferView_; }

private:

    void TransitionOutputVertex(D3D12_RESOURCE_STATES nextState);

private:

    // Matrix Palette
    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource_;
    std::span<WellForGPU> mappedPalette_;
    uint32_t paletteSrvIndex_ = 0;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle_;

    // Compute出力
    Microsoft::WRL::ComPtr<ID3D12Resource> outputVertexResource_;
    D3D12_VERTEX_BUFFER_VIEW outputVertexBufferView_{};
    uint32_t outputUavIndex_ = 0;
    D3D12_RESOURCE_STATES outputVertexState_ = D3D12_RESOURCE_STATE_COMMON;

    // 頂点数
    Microsoft::WRL::ComPtr<ID3D12Resource> skinningInfoResource_;
    SkinningInformation* skinningInfoData_ = nullptr;
    uint32_t vertexSize_ = 0;
};

