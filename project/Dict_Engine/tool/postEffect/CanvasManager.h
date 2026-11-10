#pragma once

#include "RenderCanvas.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

using CanvasId = uint32_t;

namespace BuiltinCanvas
{
	// Finalは通常の描画Canvasではなく、全Canvas合成後に適用するチェーンを表す。
	inline constexpr CanvasId Final = 0;
	inline constexpr CanvasId World = 1;
	inline constexpr CanvasId UI = 2;
}

/// <summary>
/// 複数Canvasの生成、描画先切替、PostEffect適用、最終合成を管理する。
/// </summary>
class CanvasManager
{
public:
	static CanvasManager* GetInstance();
	void Finalize();

	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class CanvasManager;
	};
	explicit CanvasManager(ConstructorKey) {}

	void Initialize(uint32_t width, uint32_t height);

	CanvasId CreateCanvas(
		const RenderCanvasDesc& desc,
		PostEffectChain* externalChain = nullptr);
	void BeginDraw(CanvasId id);
	void EndDraw();
	void ApplyPostEffects();
	void CompositeToBackBuffer();

	RenderCanvas* GetCanvas(CanvasId id);
	const RenderCanvas* GetCanvas(CanvasId id) const;
	PostEffectChain* GetPostEffectChain(CanvasId id);
	const PostEffectChain* GetPostEffectChain(CanvasId id) const;

private:
	CanvasId CreateCanvasWithId(
		CanvasId id,
		const RenderCanvasDesc& desc,
		PostEffectChain* externalChain);
	void SortCompositeOrder();
	void DrawFullscreen(
		uint32_t srvIndex,
		PSOManager::BlendMode blendMode);
	void CreateCompositeTexture();

private:
	static std::unique_ptr<CanvasManager> instance_;
	~CanvasManager() = default;
	CanvasManager(const CanvasManager&) = delete;
	CanvasManager& operator=(const CanvasManager&) = delete;
	friend struct std::default_delete<CanvasManager>;

	std::unordered_map<CanvasId, std::unique_ptr<RenderCanvas>> canvases_;
	std::vector<CanvasId> compositeOrder_;
	RenderCanvas* activeCanvas_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> compositeResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE compositeRTV_{};
	uint32_t compositeRTVIndex_ = 0;
	uint32_t compositeSRVIndex_ = 0;
	PostEffectChain finalChain_;

	CanvasId nextCanvasId_ = BuiltinCanvas::UI + 1;
	uint32_t width_ = 0;
	uint32_t height_ = 0;
	bool isReady_ = false;
};
