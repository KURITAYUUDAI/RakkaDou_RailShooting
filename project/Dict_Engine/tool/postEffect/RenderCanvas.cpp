#include "RenderCanvas.h"

#include "DirectXBase.h"
#include "PostEffectManager.h"
#include "SrvManager.h"

#include <cassert>

void RenderCanvas::Initialize(
	uint32_t width,
	uint32_t height,
	const RenderCanvasDesc& desc,
	PostEffectChain* externalChain)
{
	assert(!isReady_);
	width_ = width;
	height_ = height;
	desc_ = desc;

	CreateRenderTexture(
		desc_.clearColor,
		sourceResource_, sourceRTV_, sourceRTVIndex_, sourceSRVIndex_);
	CreateRenderTexture(
		Vector4{ 0.0f, 0.0f, 0.0f, 1.0f },
		outputResource_, outputRTV_, outputRTVIndex_, outputSRVIndex_);

	if (externalChain)
	{
		chain_ = externalChain;
	}
	else
	{
		ownedChain_ = std::make_unique<PostEffectChain>();
		PostEffectManager::GetInstance()->InitializeChain(*ownedChain_);
		chain_ = ownedChain_.get();
	}

	isReady_ = true;
}

void RenderCanvas::Finalize()
{
	if (!isReady_)
	{
		return;
	}

	if (ownedChain_)
	{
		ownedChain_->Finalize();
		ownedChain_.reset();
	}
	chain_ = nullptr;

	auto* directXBase = DirectXBase::GetInstance();
	directXBase->FreeRTVIndex(sourceRTVIndex_);
	directXBase->FreeRTVIndex(outputRTVIndex_);
	SrvManager::GetInstance()->FreeIndex(sourceSRVIndex_);
	SrvManager::GetInstance()->FreeIndex(outputSRVIndex_);

	sourceResource_.Reset();
	outputResource_.Reset();
	isReady_ = false;
}

void RenderCanvas::BeginDraw()
{
	assert(isReady_);
	DirectXBase::GetInstance()->BeginRenderTexture(
		sourceResource_.Get(), sourceRTV_, desc_.clearColor, desc_.useDepth);
}

void RenderCanvas::EndDraw()
{
	assert(isReady_);
	DirectXBase::GetInstance()->EndRenderTexture(sourceResource_.Get());
}

void RenderCanvas::ApplyPostEffects()
{
	assert(isReady_ && chain_);
	auto* directXBase = DirectXBase::GetInstance();
	directXBase->BeginRenderTexture(
		outputResource_.Get(), outputRTV_,
		Vector4{ 0.0f, 0.0f, 0.0f, 1.0f }, false);
	chain_->Draw(sourceSRVIndex_, outputRTV_);
	directXBase->EndRenderTexture(outputResource_.Get());
}

PostEffectChain& RenderCanvas::GetPostEffectChain()
{
	assert(chain_);
	return *chain_;
}

const PostEffectChain& RenderCanvas::GetPostEffectChain() const
{
	assert(chain_);
	return *chain_;
}

void RenderCanvas::CreateRenderTexture(
	const Vector4& clearColor,
	ComPtr<ID3D12Resource>& resource,
	D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
	uint32_t& rtvIndex,
	uint32_t& srvIndex)
{
	auto* directXBase = DirectXBase::GetInstance();
	auto* srvManager = SrvManager::GetInstance();

	resource = directXBase->CreateRenderTextureResource(
		width_, height_, directXBase->GetRtvFormat(), clearColor);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = directXBase->GetRtvFormat();
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvIndex = directXBase->AllocateRTVIndex();
	rtvHandle = directXBase->GetRTVCPUDescriptorHandle(rtvIndex);
	directXBase->GetDevice()->CreateRenderTargetView(
		resource.Get(), &rtvDesc, rtvHandle);

	srvIndex = srvManager->AllocateIndex();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = directXBase->GetRtvFormat();
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	directXBase->GetDevice()->CreateShaderResourceView(
		resource.Get(), &srvDesc,
		srvManager->GetCPUDescriptorHandle(srvIndex));
}
