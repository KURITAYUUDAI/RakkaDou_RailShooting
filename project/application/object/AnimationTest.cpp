#include "AnimationTest.h"
#include "CameraManager.h"
#include "UavManager.h"
#include "DebugDrawManager.h"
#include "time/DeltaTimeManager.h"

void AnimationTest::Initialize()
{
	ModelManager::GetInstance()->LoadModel("Animation", "walk.gltf");

	PSOManager::GetInstance()->RegisterSkinningPSO();

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel("walk.gltf");

	animation_ = LoadAnimationFile("Animation", "walk.gltf");
	animationTime = 0.0f;

	skeleton_ = CreateSkeleton(object3d_->GetModel()->GetRootNode(0));

	skinCluster_ = CreateSkinCluster(skeleton_, object3d_->GetModel()->GetMesh(0), skinClusterHeap_);

	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };
}

void AnimationTest::Update(float deltaTime)
{
#ifdef _DEBUG

	ImGui::Begin("Animation Test");

	EulerTransform transform = transform_;
	if (ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f))
	{
		transform_ = transform;
	}
	if (ImGui::DragFloat3("Rotate", &transform.rotate.x, 0.1f))
	{
		transform_ = transform;
	}
	if (ImGui::DragFloat3("Translate", &transform.translate.x, 0.1f))
	{
		transform_ = transform;
	}

	ImGui::End();

#endif

	animationTime += deltaTime;
	animationTime = std::fmod(animationTime, animation_.duration);

	ApplyAnimation(skeleton_, animation_, animationTime);
	UpdateSkeleton(skeleton_);
	UpdateSkinCluster(skinCluster_, skeleton_);

	object3d_->SetTransform(transform_);
	object3d_->Update(nullptr, nullptr, false);
}

void AnimationTest::Draw()
{
	object3d_->SetPsoName(psoName_);
	object3d_->SetBlendMode(blendMode_);
	object3d_->SetFillMode(fillMode_);
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	object3d_->Draw(&skinCluster_.influenceBufferView, &skinCluster_.paletteSrvHandle.second);
}

void AnimationTest::Finalize()
{}
