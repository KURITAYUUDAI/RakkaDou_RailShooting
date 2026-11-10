#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include "PostEffect.h"
#include "PostEffectChain.h"
#include "myMath.h"

#include <optional>

/// <summary>
/// ポストエフェクトを一括管理するシングルトンマネージャー
///
/// 使い方:
///   // 起動時（Game::Initialize）
///   PostEffectManager::GetInstance()->Initialize();
///
///   // エフェクトの種類を登録（起動時に1回）
///   PostEffectManager::GetInstance()->RegisterFactory(
///       "GaussianBlur", []{ return std::make_unique<GaussianBlur>(); });
///
///   // シーンのInitializeなどで適用順を設定
///   auto* pfxMgr = PostEffectManager::GetInstance();
///   pfxMgr->Clear();
///   pfxMgr->Add("Monochrome");
///   pfxMgr->Add("GaussianBlur");
///
///   // 毎フレーム DrawSwapChain() の後に呼ぶ
///   PostEffectManager::GetInstance()->Draw(
///       OffscreenRender::GetInstance()->GetRenderTextureResource(),
///       OffscreenRender::GetInstance()->GetSRVIndex());
/// </summary>
class PostEffectManager
{
public:
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    // エフェクトを生成するファクトリ関数の型
    using FactoryFunc = std::function<std::unique_ptr<PostEffect>()>;

    template<typename T>
    T* Get(const std::string& name)
    {
        return defaultChain_.Get<T>(name);
    }

public:
    // シングルトン
    static PostEffectManager* GetInstance();
    void Finalize();

    class ConstructorKey
    {
    private:
        ConstructorKey() = default;
        friend class PostEffectManager;
    };
    explicit PostEffectManager(ConstructorKey){}

public:

private:
    static std::unique_ptr<PostEffectManager> instance_;
    ~PostEffectManager() = default;
    PostEffectManager(const PostEffectManager&) = delete;
    PostEffectManager& operator=(const PostEffectManager&) = delete;
    friend struct std::default_delete<PostEffectManager>;

public:
    // -------------------------------------------------------
    //  初期化 / 終了
    // -------------------------------------------------------

    /// <summary>
    /// マネージャーの初期化。Game::Initialize() から呼ぶ。
    /// </summary>
    /// <param name="width">レンダーターゲットの幅</param>
    /// <param name="height">レンダーターゲットの高さ</param>
    void Initialize(uint32_t width, uint32_t height);

    // -------------------------------------------------------
    //  ファクトリ登録
    // -------------------------------------------------------

    /// <summary>
    /// エフェクトの生成関数を名前で登録する。
    /// Game::Initialize() でまとめて登録しておく。
    /// </summary>
    /// <param name="name">エフェクト名（Add()で使う文字列）</param>
    /// <param name="factory">エフェクトを生成するラムダ</param>
    void RegisterFactory(const std::string& name, FactoryFunc factory);

    /// <summary>
    /// Canvasが所有する独立チェーンを、登録済みFactoryと同じ条件で初期化する。
    /// </summary>
    void InitializeChain(PostEffectChain& chain);

    /// <summary>
    /// 既存のPostEffectControllerが操作するWorld用チェーン。
    /// </summary>
    PostEffectChain& GetDefaultChain() { return defaultChain_; }

    // -------------------------------------------------------
    //  エフェクトチェーンの構築（Scene::Initialize等から呼ぶ）
    // -------------------------------------------------------

   

    /// <summary>
    /// エフェクトをチェーンの末尾に追加する。
    /// 追加した順番がそのまま適用順になる。
    /// </summary>
    /// <param name="name">RegisterFactory()で登録した名前</param>
    PostEffect* Add(const std::string& name,
        std::optional<int32_t> priority = std::nullopt);

    /// <summary>
    /// 指定インデックスのエフェクトをチェーンから取り除く。
    /// </summary>
    void Remove(uint32_t index);

    void Remove(const std::string& name);

    void Remove(PostEffect* target);

    /// <summary>
    /// エフェクトの順番を入れ替える。
    /// </summary>
    void Swap(uint32_t indexA, uint32_t indexB);

    bool HasEffect(const PostEffect* target) const;
    void DebugPrintChain() const;

    // -------------------------------------------------------
    //  毎フレームの描画
    // -------------------------------------------------------

    /// <summary>
    /// チェーン順にエフェクトを適用し、最終結果をSwapChainに転写する。
    /// DrawSwapChain() の後、ImGui::Draw() の前に呼ぶ。
    /// エフェクトが0個の場合はそのままSwapChainにコピーする。
    /// </summary>
    /// <param name="srcResource">シーンRTのリソース</param>
    /// <param name="srcSRVIndex">シーンRTのSRVインデックス</param>
    void Draw(ID3D12Resource* srcResource, uint32_t srcSRVIndex);

    // -------------------------------------------------------
    //  デバッグ・情報取得
    // -------------------------------------------------------

    /// <summary>現在のチェーンに含まれるエフェクト数</summary>
    uint32_t GetEffectCount() const { return defaultChain_.GetEffectCount(); }

    /// <summary>指定インデックスのエフェクト名を返す</summary>
    const std::string& GetEffectName(uint32_t index) const;

    bool SetPriority(
     PostEffect* effect,
     const int32_t& priority);

    


private:

    // -------------------------------------------------------
    //  内部処理
    // -------------------------------------------------------

    /// <summary>
    /// エフェクトが0個のときのフォールバック描画（シーンRTをそのままSwapChainへ）
    /// </summary>
    void RegisterPassthroughPSO();
	std::unique_ptr<PostEffect> CreateEffect(const std::string& name);

    /// <summary>
   /// 現在のエフェクトチェーンをすべてクリアする。
   /// シーン遷移時に呼ぶ。
   /// </summary>
    void Clear();

public: // 外部入出力

	const PostEffect* GetEffect(uint32_t index) const
	{
		return defaultChain_.GetEffect(index);
	}

private:
    // 登録されたファクトリ関数（名前 → 生成関数）
    std::unordered_map<std::string, FactoryFunc> factories_;

    // 既存APIが操作するWorld用のデフォルトチェーン。
    // Canvas導入後もPostEffectControllerとの互換性を維持する。
    PostEffectChain defaultChain_;

    // 初期化時のRT解像度（Add()時のInitializeに渡す）
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // フォールバック用PSOの名前
    const std::string kPsoNamePassthrough_ = "PostEffectPassthrough";

    bool isReady_ = false;
};


