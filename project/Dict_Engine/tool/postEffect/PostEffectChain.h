#pragma once

#include "PostEffect.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// <summary>
/// 1つの描画面へ適用するPostEffectの並びと中間バッファを所有する。
/// 出力先をSwapChainへ固定しないため、RenderCanvasごとに独立して保持できる。
/// </summary>
class PostEffectChain
{
public:
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
	using EffectFactory =
		std::function<std::unique_ptr<PostEffect>(const std::string&)>;

	struct EffectEntry
	{
		std::unique_ptr<PostEffect> effect;
		int32_t priority = 0;
		uint64_t insertionOrder = 0;
	};

	struct PassEntry
	{
		PostEffect::PassFunc pass;
		std::vector<PostEffect::PassBarrier> barriers;
	};

public:
	void Initialize(
		uint32_t width,
		uint32_t height,
		EffectFactory effectFactory);
	void Finalize();

	PostEffect* Add(
		const std::string& name,
		std::optional<int32_t> priority = std::nullopt);
	void Remove(uint32_t index);
	void Remove(const std::string& name);
	void Remove(PostEffect* target);
	void Swap(uint32_t indexA, uint32_t indexB);
	void Clear();

	bool HasEffect(const PostEffect* target) const;
	bool SetPriority(PostEffect* target, int32_t priority);
	void DebugPrint() const;

	/// <summary>
	/// sourceへチェーンを適用し、指定されたRTVへ最終結果を描画する。
	/// destinationRTVは呼び出し側でRenderTarget状態にしておく。
	/// </summary>
	void Draw(uint32_t sourceSRVIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE destinationRTV);

	uint32_t GetEffectCount() const
	{
		return static_cast<uint32_t>(effectChain_.size());
	}
	const std::string& GetEffectName(uint32_t index) const;
	const PostEffect* GetEffect(uint32_t index) const;

	template<typename T>
	T* Get(const std::string& name)
	{
		for (auto& entry : effectChain_)
		{
			if (entry.effect->GetName() == name)
			{
				return dynamic_cast<T*>(entry.effect.get());
			}
		}
		return nullptr;
	}

private:
	void CreatePingPongBuffers(uint32_t width, uint32_t height);
	void DrawPassthrough(
		uint32_t sourceSRVIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE destinationRTV);
	void Sort();

private:
	EffectFactory effectFactory_;
	std::vector<EffectEntry> effectChain_;
	uint64_t nextInsertionOrder_ = 0;

	ComPtr<ID3D12Resource> pingPongRT_[2];
	D3D12_CPU_DESCRIPTOR_HANDLE pingPongRTV_[2] = {};
	uint32_t pingPongRTVIndex_[2] = {};
	uint32_t pingPongSRVIndex_[2] = {};

	uint32_t width_ = 0;
	uint32_t height_ = 0;
	bool isReady_ = false;

	static constexpr const char* kPassthroughPsoName_ =
		"PostEffectPassthrough";
};
