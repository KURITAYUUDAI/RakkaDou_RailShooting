#include "PostEffectManager.h"
#include "DirectXBase.h"
#include "PSOManager.h"
#include "SrvManager.h"
#include "OffscreenRender.h"
#include "Logger.h"
#include <cassert>

std::unique_ptr<PostEffectManager> PostEffectManager::instance_ = nullptr;

PostEffectManager* PostEffectManager::GetInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::make_unique<PostEffectManager>(ConstructorKey());
    }
    return instance_.get();
}

void PostEffectManager::Finalize()
{
    effectChain_.clear();
    factories_.clear();
    instance_.reset();
}

// ---------------------------------------------------------------
//  初期化
// ---------------------------------------------------------------
void PostEffectManager::Initialize(uint32_t width, uint32_t height)
{
    width_ = width;
    height_ = height;

    CreatePingPongBuffers(width, height);

    // エフェクトが0個のときのフォールバック用PSOを登録
    RegisterPassthroughPSO();

    isReady_ = true;
}

// ---------------------------------------------------------------
//  ファクトリ登録
// ---------------------------------------------------------------
void PostEffectManager::RegisterFactory(const std::string& name, FactoryFunc factory)
{
    assert(factories_.find(name) == factories_.end()
        && "PostEffectManager: 同じ名前のエフェクトが既に登録されています");

    factories_[name] = std::move(factory);
}

// ---------------------------------------------------------------
//  エフェクトチェーンの操作
// ---------------------------------------------------------------
void PostEffectManager::Clear()
{
#ifdef _DEBUG
    Logger::Log("PostEffectManager::Clear - clearing " + std::to_string(effectChain_.size()) + " effects\n");
#endif

	for (auto& chainEffect : effectChain_)
	{
		chainEffect.effect->Finalize();
	}
    effectChain_.clear();
}

PostEffect* PostEffectManager::Add(const std::string& name, 
    std::optional<int32_t> priority)
{
    auto it = factories_.find(name);
    assert(it != factories_.end()
        && "PostEffectManager: 未登録のエフェクト名です。RegisterFactory()を先に呼んでください");

    // ファクトリで生成 → 初期化 → チェーンに追加
    auto effect = it->second();
    effect->SetName(name);
    effect->Initialize(width_, height_);

    PostEffect* raw = effect.get();

    EffectEntry entry;
    entry.priority = priority.value_or(effect->GetDefaultPriority());
    entry.effect = std::move(effect);
    entry.insertionOrder = nextInsertionOrder_++;
    effectChain_.push_back(std::move(entry));

    SortEffectChain();

#ifdef _DEBUG
    // Add の最後に
    Logger::Log("PostEffectManager::Add -> ptr=" + std::to_string(reinterpret_cast<uintptr_t>(raw)) + " name=" + name + "\n");
#endif

    return raw;
}

void PostEffectManager::Remove(uint32_t index)
{
    assert(index < effectChain_.size() && "PostEffectManager: Remove インデックスが範囲外です");
    effectChain_[index].effect->Finalize();
    effectChain_.erase(effectChain_.begin() + index);
}

void PostEffectManager::Remove(const std::string& name)
{
   auto it =  std::find_if(effectChain_.begin(), effectChain_.end(), 
       [&name](const EffectEntry& entry){
            return entry.effect->GetName() == name;
       });

   // 見つかった場合は削除
   if (it != effectChain_.end())
   {
       it->effect->Finalize(); // エフェクト側の終了処理を呼び出す
       effectChain_.erase(it);
   } 
   else
   {
       // デバッグ用にログを出したり、アサートをかける場合はここに記述
       Logger::Log("PostEffectManager: Remove 対象のエフェクトが見つかりませんでした: " + name + "\n");
   }
}

void PostEffectManager::Remove(PostEffect* target)
{
#ifdef _DEBUG
    // Add の最後に
    Logger::Log("PostEffectManager::Remove(target) called ptr=" + std::to_string(reinterpret_cast<uintptr_t>(target)) + "\n");
#endif


    if (!target)
    {
        Logger::Log("PostEffectManager: Remove 対象のエフェクトが見つかりませんでした\n");
        return;
    }

    effectChain_.erase(
        std::remove_if(
            effectChain_.begin(),
            effectChain_.end(),
            [target](EffectEntry& entry){
                if (entry.effect.get() != target)
                {
                    return false;
                }

                entry.effect->Finalize();
                return true;
            }),
        effectChain_.end());
}

void PostEffectManager::Swap(uint32_t indexA, uint32_t indexB)
{
    assert(indexA < effectChain_.size() && "PostEffectManager: Swap indexA が範囲外です");
    assert(indexB < effectChain_.size() && "PostEffectManager: Swap indexB が範囲外です");
    std::swap(effectChain_[indexA], effectChain_[indexB]);
}

bool PostEffectManager::HasEffect(const PostEffect* target) const
{
    for (const auto& entry : effectChain_)
    {
        if (entry.effect.get() == target) return true;
    }
    return false;
}

void PostEffectManager::DebugPrintChain() const
{
    Logger::Log("PostEffectManager: effectChain start\n");
    for (size_t i = 0; i < effectChain_.size(); ++i)
    {
        auto p = effectChain_[i].effect.get();
        Logger::Log("  idx=" + std::to_string(i) + " ptr=" +
            std::to_string(reinterpret_cast<uintptr_t>(p)) + " name=" + p->GetName() + "\n");
    }
    Logger::Log("PostEffectManager: effectChain end\n");
}

// ---------------------------------------------------------------
//  毎フレーム描画
//
//  チェーンの流れ:
//    エフェクト0個 → DrawPassthrough（シーンRTをそのままSwapChainへ）
//    エフェクト1個 → effect[0].DrawFinal()
//    エフェクトN個 → effect[0].Draw() → ... → effect[N-1].DrawFinal()
// ---------------------------------------------------------------
void PostEffectManager::Draw(ID3D12Resource* srcResource, uint32_t srcSRVIndex)
{
    assert(isReady_);

    if (effectChain_.empty())
    {
        DrawPassthrough(srcResource, srcSRVIndex);
        return;
    }

    auto* cmdList = DirectXBase::GetInstance()->GetCommandList();

    float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    cmdList->ClearRenderTargetView(
        DirectXBase::GetInstance()->GetBackBufferRTVHandle(), clearColor, 0, nullptr);


    std::vector<PassEntry> allEntries;
    for (auto& effectEntry : effectChain_)
    {
        auto passes = effectEntry.effect->GetPasses();
		auto barriers = effectEntry.effect->GetBarriers();

        for (size_t i = 0; i < passes.size(); ++i)
        { 
            PassEntry entry;
			entry.pass = std::move(passes[i]);
			if (i < barriers.size())
			{
				entry.barriers = barriers[i];
			}
            allEntries.push_back(std::move(entry));
        }
    }


    // 現在の入力
    uint32_t        currentSRVIndex = srcSRVIndex;

    // ピンポンバッファのどちらに書くか（0 or 1）
    int dstPing = 0;

    for (size_t i = 0; i < allEntries.size(); ++i)
    {
		auto& entry = allEntries[i];
        bool isLast = (i == allEntries.size() - 1);

        for (const auto& barrier : entry.barriers)
        {
            PostEffect::TransitionResource(cmdList,
                barrier.resource, barrier.before, barrier.after);
        }

        if (isLast)
        {
            // 最後のパスはSwapChainへ
            entry.pass(DirectXBase::GetInstance()->GetBackBufferRTVHandle(), currentSRVIndex);
        } 
        else
        {
            // 中間パスはピンポンバッファへ
            PostEffect::TransitionResource(cmdList, pingPongRT_[dstPing].Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET);

            entry.pass(pingPongRTV_[dstPing], currentSRVIndex);

            PostEffect::TransitionResource(cmdList, pingPongRT_[dstPing].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            currentSRVIndex = pingPongSRVIndex_[dstPing];
            dstPing = 1 - dstPing;
        }

        for (const auto& barrier : entry.barriers)
        {
            PostEffect::TransitionResource(cmdList,
                barrier.resource, barrier.after, barrier.before);
        }
    }
}

// ---------------------------------------------------------------
//  情報取得
// ---------------------------------------------------------------
const std::string& PostEffectManager::GetEffectName(uint32_t index) const
{
    assert(index < effectChain_.size() && "PostEffectManager: GetEffectName インデックスが範囲外です");
    return effectChain_[index].effect->GetName();
}

bool PostEffectManager::SetPriority(PostEffect* target, const int32_t& priority)
{
    for (auto& entry : effectChain_)
    {
        if (entry.effect.get() == target) 
        {
            entry.priority = priority;
            SortEffectChain();
            return true;
        }
    }

    return false;
}   

void PostEffectManager::SortEffectChain()
{
    std::stable_sort(
    effectChain_.begin(),
    effectChain_.end(),
    [](const EffectEntry& a, const EffectEntry& b){
        if (a.priority != b.priority)
        {
            return a.priority < b.priority;
        }
        return a.insertionOrder < b.insertionOrder;
    });
}

// ---------------------------------------------------------------
//  フォールバック描画（エフェクト0個時）
//  シーンRTをそのままSwapChainに転写する
//  ※ 既存の OffscreenRender::Draw() と同等の処理
// ---------------------------------------------------------------
void PostEffectManager::DrawPassthrough(ID3D12Resource* srcResource, uint32_t srcSRVIndex)
{
    auto* cmdList = DirectXBase::GetInstance()->GetCommandList();
    auto* srvManager = SrvManager::GetInstance();

    auto psoSet = PSOManager::GetInstance()->GetPSOData(
        kPsoNamePassthrough_,
        PSOManager::BlendMode::None,
        PSOManager::FillMode::kSolid);

    cmdList->SetGraphicsRootSignature(psoSet.rootSignature.Get());
    cmdList->SetPipelineState(psoSet.pipelineState.Get());
    srvManager->PreDraw();
    cmdList->SetGraphicsRootDescriptorTable(
        0, srvManager->GetGPUDescriptorHandle(srcSRVIndex));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

// ---------------------------------------------------------------
//  フォールバック用PSO登録（既存の"Fullscreen"PSOと同じ構成）
// ---------------------------------------------------------------
void PostEffectManager::RegisterPassthroughPSO()
{
    PSOManager::PSOConfig config{};
    config.vertexShaderPath = L"resources/shaders/PostEffect/Fullscreen.VS.hlsl";
    config.pixelShaderPath = L"resources/shaders/PostEffect/Fullscreen.PS.hlsl";

    config.rootSignatureGenerator = []() -> ComPtr<ID3D12RootSignature>{
        D3D12_DESCRIPTOR_RANGE srvRange{};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        param.DescriptorTable.pDescriptorRanges = &srvRange;
        param.DescriptorTable.NumDescriptorRanges = 1;

        D3D12_STATIC_SAMPLER_DESC sampler =
            PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();

        D3D12_ROOT_SIGNATURE_DESC sigDesc{};
        sigDesc.NumParameters = 1;
        sigDesc.pParameters = &param;
        sigDesc.NumStaticSamplers = 1;
        sigDesc.pStaticSamplers = &sampler;
        sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> sigBlob, errBlob;
        HRESULT hr = D3D12SerializeRootSignature(
            &sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errBlob->GetBufferPointer()));
            assert(false);
        }

        ComPtr<ID3D12RootSignature> rootSig;
        hr = DirectXBase::GetInstance()->GetDevice()->CreateRootSignature(
            0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
            IID_PPV_ARGS(&rootSig));
        assert(SUCCEEDED(hr));
        return rootSig;
        };

    config.inputLayoutGenerator = [](){
        return std::vector<D3D12_INPUT_ELEMENT_DESC>{};
        };

    config.depthEnable = false;

    PSOManager::GetInstance()->RegisterPSOConfig(kPsoNamePassthrough_, config);
}

void PostEffectManager::CreatePingPongBuffers(uint32_t width, uint32_t height)
{
    auto* dxBase = DirectXBase::GetInstance();
    auto* srvManager = SrvManager::GetInstance();

    for (int i = 0; i < 2; ++i)
    {
        pingPongRT_[i] = dxBase->CreateRenderTextureResource(
            width, height, DirectXBase::GetInstance()->GetRtvFormat(),
            Vector4{ 0.0f, 0.0f, 0.0f, 1.0f });


        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DirectXBase::GetInstance()->GetRtvFormat();
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        pingPongRTVIndex_[i] = dxBase->AllocateRTVIndex();
        pingPongRTV_[i] = dxBase->GetRTVCPUDescriptorHandle(pingPongRTVIndex_[i]);
        dxBase->GetDevice()->CreateRenderTargetView(
            pingPongRT_[i].Get(), &rtvDesc, pingPongRTV_[i]);

        pingPongSRVIndex_[i] = srvManager->AllocateIndex();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DirectXBase::GetInstance()->GetRtvFormat();
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        dxBase->GetDevice()->CreateShaderResourceView(
            pingPongRT_[i].Get(), &srvDesc,
            srvManager->GetCPUDescriptorHandle(pingPongSRVIndex_[i]));
    }
}