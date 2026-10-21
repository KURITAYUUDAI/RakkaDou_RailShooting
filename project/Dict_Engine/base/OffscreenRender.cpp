#include "OffscreenRender.h"
#include "DirectXBase.h"
#include "PSOManager.h"
#include "SrvManager.h"
#include "Logger.h"
#include "WindowsAPI.h"

std::unique_ptr<OffscreenRender> OffscreenRender::instance_ = nullptr;

OffscreenRender* OffscreenRender::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<OffscreenRender>(ConstructorKey());
	}
	return instance_.get();
}

void OffscreenRender::Finalize()
{


	instance_.reset();
}

void OffscreenRender::Initialize()
{
	auto* dxBase = DirectXBase::GetInstance();

	// --- リソース生成（DirectXBaseから移動） ---
	renderTextureResource_ = dxBase->CreateRenderTextureResource(
		WindowsAPI::kClientWidth,
		WindowsAPI::kClientHeight,
		DirectXBase::GetInstance()->GetRtvFormat(),
		kRenderTargetClearValue_
	);

	// --- RTV登録（index=2をそのまま流用） ---
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DirectXBase::GetInstance()->GetRtvFormat();
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	rtvIndex_ = dxBase->AllocateRTVIndex();
	rtvHandle_ = dxBase->GetRTVCPUDescriptorHandle(rtvIndex_);
	dxBase->GetDevice()->CreateRenderTargetView(
		renderTextureResource_.Get(), &rtvDesc, rtvHandle_);

	// --- SRV登録 ---
	renderTextureSRVIndex_ = SrvManager::GetInstance()->AllocateIndex();
	/*DirectXBase::GetInstance()->SetRenderTextureSRVIndex(renderTextureSRVIndex_);*/

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DirectXBase::GetInstance()->GetRtvFormat();
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	dxBase->GetDevice()->CreateShaderResourceView(
		renderTextureResource_.Get(),
		&srvDesc,
		SrvManager::GetInstance()->GetCPUDescriptorHandle(renderTextureSRVIndex_)
	);

	// PSOの設定
	PSOManager::PSOConfig config{};
	config.vertexShaderPath = L"resources/shaders/PostEffect/Fullscreen.VS.hlsl";
	config.pixelShaderPath = L"resources/shaders/PostEffect/Fullscreen.PS.hlsl";

	// RootSignatureの設定
	config.rootSignatureGenerator = [](){
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;
		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();

		staticSamplerDescs.push_back(sampler);
		D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
		descriptorRange[0].BaseShaderRegister = 0; // t0
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE descriptorRangeE[1]{};
		descriptorRangeE[0].BaseShaderRegister = 1; // t1
		descriptorRangeE[0].NumDescriptors = 1;
		descriptorRangeE[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRangeE[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters.resize(1);

		// Enum定義 (可読性のため)
		enum {
			kTexture
		};

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);	// Tableで利用する数

		// シリアライズ
		static D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		descriptionRootSignature.pParameters = rootParameters.data();
		descriptionRootSignature.NumParameters = (UINT)rootParameters.size();
		descriptionRootSignature.pStaticSamplers = staticSamplerDescs.data();
		descriptionRootSignature.NumStaticSamplers = (UINT)staticSamplerDescs.size();


		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr)) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			assert(false);
		}

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		hr = DirectXBase::GetInstance()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		assert(SUCCEEDED(hr));

		return rootSignature;
		};

	config.inputLayoutGenerator = [](){

		return std::vector<D3D12_INPUT_ELEMENT_DESC> {};
		};

	// 深度設定
	config.depthEnable = false;

	PSOManager::GetInstance()->RegisterPSOConfig(psoNameRenderTexture_, config);

	isReady_ = true;

	OutputDebugStringA(("OffScreanRender::Initialize offscreanSRVIndex_ = "
		+ std::to_string(GetSRVIndex()) + "\n").c_str());
}

void OffscreenRender::Draw()
{
	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };	// 青っぽい色、RGBAの順
	DirectXBase::GetInstance()->GetCommandList()->
		ClearRenderTargetView(DirectXBase::GetInstance()->GetBackBufferRTVHandle(), clearColor, 0, nullptr);

	auto commandList = DirectXBase::GetInstance()->GetCommandList();

	// RenderTextureをSwapChainに転写
	auto psoSet = PSOManager::GetInstance()->GetPSOData(psoNameRenderTexture_,
		PSOManager::BlendMode::None, PSOManager::FillMode::kSolid);
	commandList->SetGraphicsRootSignature(psoSet.rootSignature.Get());
	commandList->SetPipelineState(psoSet.pipelineState.Get());
	SrvManager::GetInstance()->PreDraw();
	commandList->SetGraphicsRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUDescriptorHandle(renderTextureSRVIndex_));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}
