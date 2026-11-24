#include <cassert>
#include "MeshSkin.h"
#include "SrvManager.h"

MeshSkinBinding CreateMeshSkinBinding(const SkeletonAsset& skeletonAsset, const MeshGeometry& mesh)
{
    MeshSkinBinding binding;

    binding.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    D3D12_RANGE readRange{ 0, 0 };
   
    // Influence Resourceを作成
    binding.influenceResource = DirectXBase::GetInstance()->CreateConstantBufferResource(
        sizeof(VertexInfluence) * mesh.vertices.size());
    VertexInfluence* mappedInfluence = nullptr;
    binding.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
    std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * mesh.vertices.size());

    std::span<VertexInfluence> mappedInfluences = { mappedInfluence, mesh.vertices.size() };

    // mesh.skinClusterDataからWeightを格納
    binding.inverseBindPoseMatrices.resize(skeletonAsset.joints.size(), MakeIdentity4x4());
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
            binding.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
            for (const auto& vertexWeight : jointWeight.second.vertexWeights)
            {
                auto& currentInfluence = mappedInfluences[vertexWeight.vertexIndex];
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

    D3D12_RANGE writtenRange { 0, sizeof(VertexInfluence) *mesh.vertices.size() };
    binding.influenceResource->Unmap(0, &writtenRange);

    binding.inputVerticesSrvIndex = SrvManager::GetInstance()->AllocateIndex();

    SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
        binding.inputVerticesSrvIndex, 
        mesh.vertexResource.Get(),
        binding.vertexCount, 
        sizeof(VertexData));

    binding.influenceSrvIndex = SrvManager::GetInstance()->AllocateIndex();

    SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
        binding.influenceSrvIndex, 
        binding.influenceResource.Get(),
        binding.vertexCount, 
        sizeof(VertexInfluence));

    return binding;
}

void MeshSkin::Initialize(const SkeletonAsset& skeletonAsset, const MeshGeometry& mesh)
{
    vertexSize_ = static_cast<uint32_t>(mesh.vertices.size());

    CreatePalette(static_cast<uint32_t>(skeletonAsset.joints.size()));
    CreateOutputVertexBuffer(mesh);
    CreateSkinningInfoResource();
}

void MeshSkin::UpdatePalette(const MeshSkinBinding & binding, const SkeletonPose & skeletonPose)
{
    assert(binding.inverseBindPoseMatrices.size() == skeletonPose.GetJoints().size());

    for (uint32_t jointIndex = 0; jointIndex < skeletonPose.GetJoints().size(); ++jointIndex)
    {
        const Matrix4x4 skinningMatrix = binding.inverseBindPoseMatrices[jointIndex] 
            * skeletonPose.GetJoint(jointIndex).skeletonSpaceMatrix;

        mappedPalette_[jointIndex].skeletonSpaceMatrix = skinningMatrix;
        mappedPalette_[jointIndex].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(skinningMatrix));
    }
}

void MeshSkin::Dispatch(const MeshSkinBinding & binding, const std::string & computePsoName)
{
    auto psoSet = PSOManager::GetInstance()->GetComputePSOData(computePsoName);
    auto commandList = DirectXBase::GetInstance()->GetCommandList();

    SrvManager::GetInstance()->PreDraw();

    // COMMONまたは前フレームのVBV状態からUAVへ
    TransitionOutputVertex(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetComputeRootSignature(psoSet.rootSignature.Get());
    commandList->SetPipelineState(psoSet.pipelineState.Get());
    commandList->SetComputeRootDescriptorTable(0, paletteSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, SrvManager::GetInstance()->GetGPUDescriptorHandle(binding.inputVerticesSrvIndex));
    commandList->SetComputeRootDescriptorTable(2, SrvManager::GetInstance()->GetGPUDescriptorHandle(binding.influenceSrvIndex));
    commandList->SetComputeRootDescriptorTable(3, SrvManager::GetInstance()->GetGPUDescriptorHandle(outputUavIndex_));
    commandList->SetComputeRootConstantBufferView(4, skinningInfoResource_->GetGPUVirtualAddress());

    commandList->Dispatch(UINT(vertexSize_ + 1023) / 1024, 1, 1);

    // Compute書き込み後、頂点バッファとして読める状態へ
    TransitionOutputVertex(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void MeshSkin::CreatePalette(uint32_t jointSize)
{
    // palette用のResourceを確保
    paletteResource_ = DirectXBase::GetInstance()->CreateConstantBufferResource(
        sizeof(WellForGPU) * jointSize);
    WellForGPU* mappedPalette = nullptr;
    paletteResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
    mappedPalette_ = { mappedPalette, jointSize };
    paletteSrvIndex_ = SrvManager::GetInstance()->AllocateIndex();
    paletteSrvHandle_.first = SrvManager::GetInstance()->GetCPUDescriptorHandle(paletteSrvIndex_);
    paletteSrvHandle_.second = SrvManager::GetInstance()->GetGPUDescriptorHandle(paletteSrvIndex_);

    // palette用のsrvを作成。StructuredBufferでアクセスできるようにする。
    D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
    paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    paletteSrvDesc.Buffer.FirstElement = 0;
    paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    paletteSrvDesc.Buffer.NumElements = UINT(jointSize);
    paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
    DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(paletteResource_.Get(),
        &paletteSrvDesc, paletteSrvHandle_.first);
}

void MeshSkin::CreateOutputVertexBuffer(const MeshGeometry & mesh)
{
    UINT sizeInBytes = static_cast<UINT>(sizeof(VertexData) * mesh.vertices.size());

    outputVertexResource_ = DirectXBase::GetInstance()->CreateUnorderedResource(sizeInBytes);

    outputVertexBufferView_.BufferLocation = outputVertexResource_->GetGPUVirtualAddress();
    outputVertexBufferView_.SizeInBytes = sizeInBytes;
    outputVertexBufferView_.StrideInBytes = sizeof(VertexData);

    outputUavIndex_ = SrvManager::GetInstance()->AllocateIndex();

    SrvManager::GetInstance()->CreateUAVforStructuredBuffer(
        outputUavIndex_,
        outputVertexResource_.Get(),
        static_cast<UINT>(mesh.vertices.size()),
        sizeof(VertexData)
    );
}

void MeshSkin::CreateSkinningInfoResource()
{
    skinningInfoResource_ = DirectXBase::GetInstance()->CreateConstantBufferResource(sizeof(SkinningInformation));

    D3D12_RANGE readRange{ 0, 0 };
    skinningInfoResource_->Map(0, &readRange, reinterpret_cast<void**>(&skinningInfoData_));

    skinningInfoData_->numVertices = static_cast<uint32_t>(vertexSize_);
}

void MeshSkin::TransitionOutputVertex(D3D12_RESOURCE_STATES nextState)
{
    if (outputVertexState_ == nextState)
    {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    barrier.Transition.pResource = outputVertexResource_.Get();
    barrier.Transition.StateBefore = outputVertexState_;
    barrier.Transition.StateAfter = nextState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    DirectXBase::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);

    outputVertexState_ = nextState;
}
