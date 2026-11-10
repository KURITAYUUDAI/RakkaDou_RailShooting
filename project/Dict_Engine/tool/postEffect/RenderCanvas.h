#pragma once

#include "PostEffectChain.h"
#include "PSOManager.h"
#include "myMath.h"

#include <cstdint>
#include <memory>
#include <string>

struct RenderCanvasDesc
{
	std::string name;
	int32_t compositeOrder = 0;
	Vector4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	bool useDepth = false;
	PSOManager::BlendMode compositeBlend = PSOManager::BlendMode::Normal;
};

/// <summary>
/// 描画用Texture、PostEffect適用後Texture、EffectChainを一組として管理する。
/// </summary>
class RenderCanvas
{
public:
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	void Initialize(
		uint32_t width,
		uint32_t height,
		const RenderCanvasDesc& desc,
		PostEffectChain* externalChain = nullptr);
	void Finalize();

	void BeginDraw();
	void EndDraw();
	void ApplyPostEffects();

	PostEffectChain& GetPostEffectChain();
	const PostEffectChain& GetPostEffectChain() const;
	const RenderCanvasDesc& GetDesc() const { return desc_; }

	ID3D12Resource* GetSourceResource() const
	{
		return sourceResource_.Get();
	}
	uint32_t GetSourceSRVIndex() const { return sourceSRVIndex_; }

	ID3D12Resource* GetOutputResource() const
	{
		return outputResource_.Get();
	}
	uint32_t GetOutputSRVIndex() const { return outputSRVIndex_; }

	bool IsReady() const { return isReady_; }

private:
	void CreateRenderTexture(
		const Vector4& clearColor,
		ComPtr<ID3D12Resource>& resource,
		D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
		uint32_t& rtvIndex,
		uint32_t& srvIndex);

private:
	RenderCanvasDesc desc_;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

	ComPtr<ID3D12Resource> sourceResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE sourceRTV_{};
	uint32_t sourceRTVIndex_ = 0;
	uint32_t sourceSRVIndex_ = 0;

	ComPtr<ID3D12Resource> outputResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE outputRTV_{};
	uint32_t outputRTVIndex_ = 0;
	uint32_t outputSRVIndex_ = 0;

	std::unique_ptr<PostEffectChain> ownedChain_;
	PostEffectChain* chain_ = nullptr;
	bool isReady_ = false;
};
