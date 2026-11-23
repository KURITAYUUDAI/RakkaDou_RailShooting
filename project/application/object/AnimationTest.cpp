#include "AnimationTest.h"
#include "CameraManager.h"
#include "DebugDrawManager.h"
#include "time/DeltaTimeManager.h"
#include "SrvManager.h"


void AnimationTest::Initialize()
{
	ModelManager::GetInstance()->LoadModel("Animation/Sample", "WireShotSample.gltf");

	PSOManager::GetInstance()->RegisterSkinningPSO();

	PSOManager::GetInstance()->RegisterSkinningComputePSO();

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel("WireShotSample.gltf");

	animationSet_ = LoadAnimationSet("Animation/Sample", "WireShotSample.gltf");
	animationPlayer_.Play(animationSet_.GetAnimation(0), true);

	skeletonAsset_ = CreateSkeletonAsset(object3d_->GetModel()->GetRootNode(0));
	skeletonPose_.Initialize(&skeletonAsset_);
	
	skinBinding_ = CreateMeshSkinBinding(skeletonAsset_, object3d_->GetModel()->GetMesh(0));
	meshSkin_.Initialize(skeletonAsset_, object3d_->GetModel()->GetMesh(0));

	/*skinCluster_ = CreateSkinCluster(skeletonAsset_, object3d_->GetModel()->GetMesh(0), skinClusterHeap_);
	CreateInputVerticesSRV();
	CreateInfluenceSRV();
	CreateOutputVertexBuffer();
	CreateSkinningInfoResource();*/

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

	float dissolveThreshold = dissolveParams_.threshold;
	Vector4 dissolveEdgeColor = dissolveParams_.edgeColor;
	if (ImGui::SliderFloat("threshold", &dissolveThreshold, 0.0f, 1.0f))
	{
		SetThreshold(dissolveThreshold);
	}
	if (ImGui::DragFloat4("EdgeColor", &dissolveEdgeColor.x, 0.1f, 0.0f, 10.0f))
	{
		SetEdgeColor(dissolveEdgeColor);
	}

	ImGui::End();

#endif

	animationPlayer_.Update(deltaTime);

	skeletonPose_.ResetToBindPose();

	const Animation* animation = animationPlayer_.GetCurrentAnimation();

	if (animation != nullptr)
	{
		skeletonPose_.ApplyAnimation(*animation, animationPlayer_.GetCurrentTime());
	}

	skeletonPose_.Update();
	meshSkin_.UpdatePalette(skinBinding_, skeletonPose_);

	object3d_->SetTransform(transform_);
	object3d_->Update(nullptr, nullptr, false);
}

void AnimationTest::Draw()
{
	meshSkin_.Dispatch(skinBinding_, computePsoName_);

	object3d_->SetPsoName(psoName_);
	object3d_->SetBlendMode(blendMode_);
	object3d_->SetFillMode(fillMode_);
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(5, environmentTextureIndex_);

	DissolveManager::GetInstance()->SetCbufferDissolveResource(6, dissolveParams_);
	DissolveManager::GetInstance()->SetCbufferMaskTexture(7, 0);

	//object3d_->Draw(&skinCluster_.influenceBufferView, &skinCluster_.paletteSrvHandle.second);

	object3d_->DrawWithOverrideVertexBuffer(meshSkin_.GetOutputVertexBufferView());
}

void AnimationTest::Finalize()
{}
