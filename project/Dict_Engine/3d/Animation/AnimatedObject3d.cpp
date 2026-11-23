#include "AnimatedObject3d.h"
#include "ModelManager.h"

std::shared_ptr<AnimatedModelAsset> CreateAnimatedModelAsset(const std::string& directoryPath, const std::string& filename)
{
    ModelManager::GetInstance()->LoadModel(directoryPath, filename);

    auto asset = std::make_shared<AnimatedModelAsset>();

    asset->modelName = filename;
    asset->model =ModelManager::GetInstance()->FindModel(filename);

    assert(asset->model != nullptr);

    asset->animationSet = LoadAnimationSet(directoryPath, filename);
    asset->skeletonAsset = CreateSkeletonAsset(asset->model->GetRootNode(0));

    const uint32_t meshCount = asset->model->GetMeshCount();

    asset->skinBindings.resize(meshCount);

    for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
    {
        MeshGeometry& mesh = asset->model->GetMesh(meshIndex);

        if (mesh.skinClusterData.empty())
        {
            continue;
        }

        asset->skinBindings[meshIndex] = CreateMeshSkinBinding(asset->skeletonAsset,mesh);
    }

    return asset;
}

void AnimatedObject3d::Initialize(std::shared_ptr<const AnimatedModelAsset> asset)
{
    assert(asset != nullptr);
    assert(asset->model != nullptr);

    asset_ = std::move(asset);

    object3d_.Initialize();
    object3d_.SetModel(asset_->modelName);

    skeletonPose_.Initialize(&asset_->skeletonAsset);

    const uint32_t meshCount = asset_->model->GetMeshCount();

    meshSkins_.resize(meshCount);

    for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
    {
        if (!asset_->skinBindings[meshIndex]) { continue; }

        meshSkins_[meshIndex] = std::make_unique<MeshSkin>();
        meshSkins_[meshIndex]->Initialize(asset_->skeletonAsset, asset_->model->GetMesh(meshIndex));
    }
}

void AnimatedObject3d::Update(float deltaTime)
{
    if (!asset_)
    {
        return;
    }

    animationPlayer_.Update(deltaTime);
    skeletonPose_.ResetToBindPose();

    if (const Animation* animation = animationPlayer_.GetCurrentAnimation())
    {
        skeletonPose_.ApplyAnimation(*animation,animationPlayer_.GetCurrentTime());
    }

    skeletonPose_.Update();
    UpdateSkinningPalette();

    // Skinning後の頂点はすでにモデル座標なのでRootNodeを再度掛けない
    object3d_.Update(nullptr, nullptr, false);
}

void AnimatedObject3d::PreDraw()
{
    DispatchSkinning();
    object3d_.PreDraw();
}

void AnimatedObject3d::Draw()
{
    if (!asset_)
    {
        return;
    }

    const uint32_t meshCount = asset_->model->GetMeshCount();

    for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
    {
        if (meshSkins_[meshIndex])
        {
            const auto& vbv = meshSkins_[meshIndex]->GetOutputVertexBufferView();
            object3d_.DrawMesh(meshIndex, &vbv);
        } 
        else
        {
            object3d_.DrawMesh(meshIndex, nullptr);
        }
    }
}

void AnimatedObject3d::PlayAnimation(uint32_t animationIndex, bool isLoop)
{
    if (!asset_)
    {
        return;
    }

    const Animation* animation = asset_->animationSet.GetAnimation(animationIndex);

    animationPlayer_.Play(animation,isLoop);
}

void AnimatedObject3d::PlayAnimation(const std::string & animationName, bool isLoop)
{
    if (!asset_)
    {
        return;
    }

    const Animation* animation = asset_->animationSet.FindAnimation(animationName);

    animationPlayer_.Play(animation,isLoop);
}

void AnimatedObject3d::StopAnimation()
{}

void AnimatedObject3d::PauseAnimation()
{}

void AnimatedObject3d::ResumeAnimation()
{}

//void AnimatedObject3d::UpdateAnimation()
//{}

void AnimatedObject3d::UpdateSkinningPalette()
{
    for (uint32_t meshIndex = 0; meshIndex < meshSkins_.size(); ++meshIndex)
    {
        if (!meshSkins_[meshIndex] || !asset_->skinBindings[meshIndex])
        {
            continue;
        }

        meshSkins_[meshIndex]->UpdatePalette(*asset_->skinBindings[meshIndex],skeletonPose_);
    }
}

void AnimatedObject3d::DispatchSkinning()
{
    for (uint32_t meshIndex = 0;meshIndex < meshSkins_.size(); ++meshIndex)
    {
        if (!meshSkins_[meshIndex] || !asset_->skinBindings[meshIndex])
        {
            continue;
        }

        meshSkins_[meshIndex]->Dispatch(*asset_->skinBindings[meshIndex], computePsoName_);
    }
}
