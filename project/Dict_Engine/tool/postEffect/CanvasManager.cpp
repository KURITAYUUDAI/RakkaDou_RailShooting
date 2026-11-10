#include "CanvasManager.h"

#include "DirectXBase.h"
#include "PostEffectManager.h"
#include "SrvManager.h"

#include <algorithm>
#include <cassert>

std::unique_ptr<CanvasManager> CanvasManager::instance_ = nullptr;

CanvasManager* CanvasManager::GetInstance()
{
	if (!instance_)
	{
		instance_ = std::make_unique<CanvasManager>(ConstructorKey());
	}
	return instance_.get();
}

void CanvasManager::Initialize(uint32_t width, uint32_t height)
{
	assert(!isReady_);
	width_ = width;
	height_ = height;
	CreateCompositeTexture();
	PostEffectManager::GetInstance()->InitializeChain(finalChain_);

	RenderCanvasDesc worldDesc;
	worldDesc.name = "World";
	worldDesc.compositeOrder = 0;
	worldDesc.clearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
	worldDesc.useDepth = true;
	worldDesc.compositeBlend = PSOManager::BlendMode::None;
	CreateCanvasWithId(
		BuiltinCanvas::World,
		worldDesc,
		&PostEffectManager::GetInstance()->GetDefaultChain());

	RenderCanvasDesc uiDesc;
	uiDesc.name = "UI";
	uiDesc.compositeOrder = 100;
	uiDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	uiDesc.useDepth = false;
	uiDesc.compositeBlend = PSOManager::BlendMode::Normal;
	CreateCanvasWithId(BuiltinCanvas::UI, uiDesc, nullptr);

	isReady_ = true;
}

void CanvasManager::Finalize()
{
	activeCanvas_ = nullptr;
	for (auto& [id, canvas] : canvases_)
	{
		(void)id;
		canvas->Finalize();
	}
	canvases_.clear();
	compositeOrder_.clear();

	finalChain_.Finalize();
	DirectXBase::GetInstance()->FreeRTVIndex(compositeRTVIndex_);
	SrvManager::GetInstance()->FreeIndex(compositeSRVIndex_);
	compositeResource_.Reset();

	isReady_ = false;
	instance_.reset();
}

CanvasId CanvasManager::CreateCanvas(
	const RenderCanvasDesc& desc,
	PostEffectChain* externalChain)
{
	const CanvasId id = nextCanvasId_++;
	return CreateCanvasWithId(id, desc, externalChain);
}

void CanvasManager::BeginDraw(CanvasId id)
{
	assert(isReady_ && !activeCanvas_);
	activeCanvas_ = GetCanvas(id);
	assert(activeCanvas_);
	activeCanvas_->BeginDraw();
}

void CanvasManager::EndDraw()
{
	assert(activeCanvas_);
	activeCanvas_->EndDraw();
	activeCanvas_ = nullptr;
}

void CanvasManager::ApplyPostEffects()
{
	assert(isReady_ && !activeCanvas_);
	for (CanvasId id : compositeOrder_)
	{
		canvases_.at(id)->ApplyPostEffects();
	}
}

void CanvasManager::CompositeToBackBuffer()
{
	assert(isReady_ && !activeCanvas_);
	auto* directXBase = DirectXBase::GetInstance();

	// 各Canvasを一度Textureへ合成し、その結果をFinal Chainの入力にする。
	directXBase->BeginRenderTexture(
		compositeResource_.Get(), compositeRTV_,
		Vector4{ 0.0f, 0.0f, 0.0f, 1.0f }, false);

	for (CanvasId id : compositeOrder_)
	{
		const auto& canvas = *canvases_.at(id);
		DrawFullscreen(
			canvas.GetOutputSRVIndex(),
			canvas.GetDesc().compositeBlend);
	}

	directXBase->EndRenderTexture(compositeResource_.Get());
	directXBase->BeginBackBuffer();
	finalChain_.Draw(
		compositeSRVIndex_, directXBase->GetBackBufferRTVHandle());
}

RenderCanvas* CanvasManager::GetCanvas(CanvasId id)
{
	const auto iterator = canvases_.find(id);
	return iterator != canvases_.end() ? iterator->second.get() : nullptr;
}

const RenderCanvas* CanvasManager::GetCanvas(CanvasId id) const
{
	const auto iterator = canvases_.find(id);
	return iterator != canvases_.end() ? iterator->second.get() : nullptr;
}

PostEffectChain* CanvasManager::GetPostEffectChain(CanvasId id)
{
	if (id == BuiltinCanvas::Final)
	{
		return &finalChain_;
	}
	RenderCanvas* canvas = GetCanvas(id);
	return canvas ? &canvas->GetPostEffectChain() : nullptr;
}

const PostEffectChain* CanvasManager::GetPostEffectChain(CanvasId id) const
{
	if (id == BuiltinCanvas::Final)
	{
		return &finalChain_;
	}
	const RenderCanvas* canvas = GetCanvas(id);
	return canvas ? &canvas->GetPostEffectChain() : nullptr;
}

CanvasId CanvasManager::CreateCanvasWithId(
	CanvasId id,
	const RenderCanvasDesc& desc,
	PostEffectChain* externalChain)
{
	assert(canvases_.find(id) == canvases_.end());
	auto canvas = std::make_unique<RenderCanvas>();
	canvas->Initialize(width_, height_, desc, externalChain);
	canvases_.emplace(id, std::move(canvas));
	compositeOrder_.push_back(id);
	SortCompositeOrder();
	return id;
}

void CanvasManager::SortCompositeOrder()
{
	std::stable_sort(
		compositeOrder_.begin(), compositeOrder_.end(),
		[this](CanvasId left, CanvasId right)
		{
			return canvases_.at(left)->GetDesc().compositeOrder <
				canvases_.at(right)->GetDesc().compositeOrder;
		});
}

void CanvasManager::DrawFullscreen(
	uint32_t srvIndex,
	PSOManager::BlendMode blendMode)
{
	auto* commandList = DirectXBase::GetInstance()->GetCommandList();
	auto pso = PSOManager::GetInstance()->GetPSOData(
		"PostEffectPassthrough", blendMode, PSOManager::FillMode::kSolid);
	commandList->SetGraphicsRootSignature(pso.rootSignature.Get());
	commandList->SetPipelineState(pso.pipelineState.Get());
	SrvManager::GetInstance()->PreDraw();
	commandList->SetGraphicsRootDescriptorTable(
		0, SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void CanvasManager::CreateCompositeTexture()
{
	auto* directXBase = DirectXBase::GetInstance();
	auto* srvManager = SrvManager::GetInstance();
	const Vector4 clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

	compositeResource_ = directXBase->CreateRenderTextureResource(
		width_, height_, directXBase->GetRtvFormat(), clearColor);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = directXBase->GetRtvFormat();
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	compositeRTVIndex_ = directXBase->AllocateRTVIndex();
	compositeRTV_ = directXBase->GetRTVCPUDescriptorHandle(
		compositeRTVIndex_);
	directXBase->GetDevice()->CreateRenderTargetView(
		compositeResource_.Get(), &rtvDesc, compositeRTV_);

	compositeSRVIndex_ = srvManager->AllocateIndex();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = directXBase->GetRtvFormat();
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	directXBase->GetDevice()->CreateShaderResourceView(
		compositeResource_.Get(), &srvDesc,
		srvManager->GetCPUDescriptorHandle(compositeSRVIndex_));
}
