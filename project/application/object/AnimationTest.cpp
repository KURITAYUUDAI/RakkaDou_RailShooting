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
	animation_ = animationSet_.GetAnimation(0);

	animationTime = 0.0f;

	skeleton_ = CreateSkeleton(object3d_->GetModel()->GetRootNode(0));
	skinCluster_ = CreateSkinCluster(skeleton_, object3d_->GetModel()->GetMesh(0), skinClusterHeap_);
	CreateInputVerticesSRV();
	CreateInfluenceSRV();
	CreateOutputVertexBuffer();
	CreateSkinningInfoResource();

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

	if (animation_ != nullptr)
	{
		animationTime += deltaTime;

		if (animation_->duration > 0.0f)
		{
			animationTime = std::fmod(animationTime, animation_->duration);
		}

		ApplyAnimation(skeleton_, *animation_, animationTime);
	}

	UpdateSkeleton(skeleton_);
	UpdateSkinCluster(skinCluster_, skeleton_);

	object3d_->SetTransform(transform_);
	object3d_->Update(nullptr, nullptr, false);
}

void AnimationTest::Draw()
{
	DispatchSkinning();

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

	object3d_->DrawWithOverrideVertexBuffer(outputVertexBufferView);
}

void AnimationTest::Finalize()
{}

void AnimationTest::CreateInputVerticesSRV()
{
	inputVerticesSrvIndex_ = SrvManager::GetInstance()->AllocateIndex();

	MeshGeometry& mesh = object3d_->GetModel()->GetMesh(0);

	SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
		inputVerticesSrvIndex_,
		mesh.vertexResource.Get(),
		static_cast<UINT>(mesh.vertices.size()),
		sizeof(VertexData));
}

void AnimationTest::CreateInfluenceSRV()
{
	influenceSrvIndex_ = SrvManager::GetInstance()->AllocateIndex();

	MeshGeometry& mesh = object3d_->GetModel()->GetMesh(0);

	SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
		influenceSrvIndex_,
		skinCluster_.influenceResource.Get(),
		static_cast<UINT>(mesh.vertices.size()),
		sizeof(VertexInfluence)
	);
}

void AnimationTest::CreateOutputVertexBuffer()
{
	MeshGeometry& mesh = object3d_->GetModel()->GetMesh(0);
	UINT sizeInBytes = static_cast<UINT>(sizeof(VertexData) * mesh.vertices.size());

	outputVertexResource_ = DirectXBase::GetInstance()->
		CreateUnorderedResource(sizeInBytes);
	
	outputVertexBufferView.BufferLocation = outputVertexResource_->GetGPUVirtualAddress();
	outputVertexBufferView.SizeInBytes = sizeInBytes;
	outputVertexBufferView.StrideInBytes = sizeof(VertexData);
	
	outputUavIndex_ = SrvManager::GetInstance()->AllocateIndex();

	SrvManager::GetInstance()->CreateUAVforStructuredBuffer(
		outputUavIndex_, 
		outputVertexResource_.Get(), 
		static_cast<UINT>(mesh.vertices.size()),
		sizeof(VertexData)
	);
	
}

void AnimationTest::CreateSkinningInfoResource()
{
	skinningInfoResource_ = DirectXBase::GetInstance()->CreateConstantBufferResource(sizeof(SkinningInformation));

	skinningInfoResource_->Map(0, nullptr, reinterpret_cast<void**>(&skinningInfoData_));

	skinningInfoData_->numVertices = static_cast<uint32_t>(object3d_->GetModel()->GetMesh(0).vertices.size());
	
}

void AnimationTest::DispatchSkinning()
{
	auto psoSet = PSOManager::GetInstance()->GetComputePSOData(computePsoName_);
	auto commandList = DirectXBase::GetInstance()->GetCommandList();
	
	SrvManager::GetInstance()->PreDraw();

	// COMMONまたは前フレームのVBV状態からUAVへ
	TransitionOutputVertex(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootSignature(psoSet.rootSignature.Get());
	commandList->SetPipelineState(psoSet.pipelineState.Get());
	commandList->SetComputeRootDescriptorTable(0, skinCluster_.paletteSrvHandle.second);
	commandList->SetComputeRootDescriptorTable(1, SrvManager::GetInstance()->GetGPUDescriptorHandle(inputVerticesSrvIndex_));
	commandList->SetComputeRootDescriptorTable(2, SrvManager::GetInstance()->GetGPUDescriptorHandle(influenceSrvIndex_));
	commandList->SetComputeRootDescriptorTable(3, SrvManager::GetInstance()->GetGPUDescriptorHandle(outputUavIndex_));
	commandList->SetComputeRootConstantBufferView(4, skinningInfoResource_->GetGPUVirtualAddress());

	commandList->Dispatch(UINT(object3d_->GetModel()->GetMesh(0).vertices.size() + 1023) / 1024, 1, 1);

	// Compute書き込み後、頂点バッファとして読める状態へ
	TransitionOutputVertex(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void AnimationTest::TransitionOutputVertex(D3D12_RESOURCE_STATES nextState)
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
