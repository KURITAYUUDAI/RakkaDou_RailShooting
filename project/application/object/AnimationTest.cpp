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

	animatedModelAsset_ = CreateAnimatedModelAsset("Animation/Sample", "WireShotSample.gltf");

	animatedObject3d_ = std::make_unique<AnimatedObject3d>();
	animatedObject3d_->Initialize(animatedModelAsset_);
	animatedObject3d_->SetModel("WireShotSample.gltf");

	animatedObject3d_->PlayAnimation(0, true);

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

	animatedObject3d_->SetTransform(transform_);
	animatedObject3d_->Update(deltaTime);
}

void AnimationTest::Draw()
{
	animatedObject3d_->SetPsoName(psoName_);
	animatedObject3d_->SetBlendMode(blendMode_);
	animatedObject3d_->SetFillMode(fillMode_);
	
	animatedObject3d_->PreDraw();

	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoName_, blendMode_, fillMode_);

	// パイプラインステートとルートシグネチャをセット
	DirectXBase::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	DirectXBase::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	animatedObject3d_->PreDraw();

	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(5, environmentTextureIndex_);

	DissolveManager::GetInstance()->SetCbufferDissolveResource(6, dissolveParams_);
	DissolveManager::GetInstance()->SetCbufferMaskTexture(7, 0);

	animatedObject3d_->Draw();
} 

void AnimationTest::Finalize()
{}
