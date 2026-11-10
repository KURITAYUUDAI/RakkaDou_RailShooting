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
    defaultChain_.Finalize();
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

    // エフェクトが0個のときのフォールバック用PSOを登録
    RegisterPassthroughPSO();

    InitializeChain(defaultChain_);

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

void PostEffectManager::InitializeChain(PostEffectChain& chain)
{
    chain.Initialize(
        width_, height_,
        [this](const std::string& name)
        {
            return CreateEffect(name);
        });
}

// ---------------------------------------------------------------
//  エフェクトチェーンの操作
// ---------------------------------------------------------------
void PostEffectManager::Clear()
{
#ifdef _DEBUG
    Logger::Log("PostEffectManager::Clear - clearing " +
        std::to_string(defaultChain_.GetEffectCount()) + " effects\n");
#endif
    defaultChain_.Clear();
}

PostEffect* PostEffectManager::Add(const std::string& name, 
    std::optional<int32_t> priority)
{
    PostEffect* raw = defaultChain_.Add(name, priority);

#ifdef _DEBUG
    // Add の最後に
    Logger::Log("PostEffectManager::Add -> ptr=" + std::to_string(reinterpret_cast<uintptr_t>(raw)) + " name=" + name + "\n");
#endif

    return raw;
}

void PostEffectManager::Remove(uint32_t index)
{
    defaultChain_.Remove(index);
}

void PostEffectManager::Remove(const std::string& name)
{
    defaultChain_.Remove(name);
}

void PostEffectManager::Remove(PostEffect* target)
{
#ifdef _DEBUG
    // Add の最後に
    Logger::Log("PostEffectManager::Remove(target) called ptr=" + std::to_string(reinterpret_cast<uintptr_t>(target)) + "\n");
#endif


    defaultChain_.Remove(target);
}

void PostEffectManager::Swap(uint32_t indexA, uint32_t indexB)
{
    defaultChain_.Swap(indexA, indexB);
}

bool PostEffectManager::HasEffect(const PostEffect* target) const
{
    return defaultChain_.HasEffect(target);
}

void PostEffectManager::DebugPrintChain() const
{
    defaultChain_.DebugPrint();
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
    (void)srcResource;
    defaultChain_.Draw(
        srcSRVIndex,
        DirectXBase::GetInstance()->GetBackBufferRTVHandle());
}

// ---------------------------------------------------------------
//  情報取得
// ---------------------------------------------------------------
const std::string& PostEffectManager::GetEffectName(uint32_t index) const
{
    return defaultChain_.GetEffectName(index);
}

bool PostEffectManager::SetPriority(PostEffect* target, const int32_t& priority)
{
    return defaultChain_.SetPriority(target, priority);
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

std::unique_ptr<PostEffect> PostEffectManager::CreateEffect(
    const std::string& name)
{
    const auto iterator = factories_.find(name);
    assert(iterator != factories_.end() &&
        "PostEffectManager: 未登録のエフェクト名です");
    if (iterator == factories_.end())
    {
        return nullptr;
    }
    return iterator->second();
}
