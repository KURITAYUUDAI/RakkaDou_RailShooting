#include "Outline.h"
#include "DirectXBase.h"
#include "PSOManager.h"
#include "SrvManager.h"
#include "Logger.h"
#include <cassert>

#include "CameraManager.h"

void Outline::Initialize(uint32_t width, uint32_t height)
{
    CreateDepthStencilResource();

    RegisterPSOs();
}

void Outline::Finalize()
{}

std::vector<PostEffect::PassFunc> Outline::GetPasses()
{
    return 
    {
        [this](D3D12_CPU_DESCRIPTOR_HANDLE destRTV, uint32_t srcSRVIndex)
        {
            Pass(destRTV, srcSRVIndex);
        }
    };
}

std::vector<std::vector<PostEffect::PassBarrier>> Outline::GetBarriers()
{
    return 
    {
        {
            {
                depthStencilResource_,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,           // before
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // after
            }
        }
    };
}

void Outline::Pass(D3D12_CPU_DESCRIPTOR_HANDLE destRTV, uint32_t srcSRVIndex)
{
    auto* cmdList = DirectXBase::GetInstance()->GetCommandList();
    auto* srvManager = SrvManager::GetInstance();

    float clearColor[4] = {
        kClearColor_.x, kClearColor_.y, kClearColor_.z, kClearColor_.w };
    cmdList->ClearRenderTargetView(destRTV, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

	auto pso = PSOManager::GetInstance()->GetPSOData(
		kPsoName_, PSOManager::BlendMode::None, PSOManager::FillMode::kSolid);
	cmdList->SetGraphicsRootSignature(pso.rootSignature.Get());
	cmdList->SetPipelineState(pso.pipelineState.Get());
	srvManager->PreDraw();
	cmdList->SetGraphicsRootDescriptorTable(
		0, srvManager->GetGPUDescriptorHandle(srcSRVIndex));
	cmdList->SetGraphicsRootDescriptorTable(
		1, srvManager->GetGPUDescriptorHandle(depthStencilSrvIndex_));

	CameraManager::GetInstance()->SetCbufferProjectionInverseResource(2);
	
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

void Outline::RegisterPSOs()
{
    auto makeRootSignature = []() -> ComPtr<ID3D12RootSignature>{
        D3D12_DESCRIPTOR_RANGE srvRange{};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0; // t0
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE depthRange{};
        depthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        depthRange.NumDescriptors = 1;
        depthRange.BaseShaderRegister = 1; // t1
        depthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[3]{};
        // [0] t0 : 入力テクスチャ
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
        rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
        // [1] t1 : Depthを読むためのテクスチャ
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &depthRange;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		// [2] b0 : カメラの情報
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
        rootParameters[2].Descriptor.ShaderRegister = 0;	// レジスタ番号0を使う

        // バイリニア + クランプ（線形サンプリング最適化に必須）
        D3D12_STATIC_SAMPLER_DESC sampler[2]{};
        sampler[0] = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();
        sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler[0].ShaderRegister = 0; // s0
        sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

		sampler[1] = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc(); 
		sampler[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler[1].ShaderRegister = 1; // s1
		sampler[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        D3D12_ROOT_SIGNATURE_DESC sigDesc{};
        sigDesc.NumParameters = _countof(rootParameters);
        sigDesc.pParameters = rootParameters;
        sigDesc.NumStaticSamplers = 2;
		sigDesc.pStaticSamplers = sampler;
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

    auto makeInputLayout = [](){
        return std::vector<D3D12_INPUT_ELEMENT_DESC>{};
        };

    PSOManager::PSOConfig configH{};
    configH.vertexShaderPath = L"resources/shaders/PostEffect/Fullscreen.VS.hlsl";
    configH.pixelShaderPath = L"resources/shaders/PostEffect/DepthBasedOutline.PS.hlsl";
    configH.rootSignatureGenerator = makeRootSignature;
    configH.inputLayoutGenerator = makeInputLayout;
    configH.depthEnable = false;
    PSOManager::GetInstance()->RegisterPSOConfig(kPsoName_, configH);

}

void Outline::CreateDepthStencilResource()
{
	depthStencilSrvIndex_ = SrvManager::GetInstance()->AllocateIndex();

	depthStencilResource_ = DirectXBase::GetInstance()->GetDepthResource();

    D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc{};
    depthTextureSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthTextureSrvDesc.Texture2D.MipLevels = 1;
    DirectXBase::GetInstance()->GetDevice()->CreateShaderResourceView(
        depthStencilResource_, &depthTextureSrvDesc,
        SrvManager::GetInstance()->GetCPUDescriptorHandle(depthStencilSrvIndex_));
}


