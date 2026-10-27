#include "Vignette.h"
#include "DirectXBase.h"
#include "PSOManager.h"
#include "SrvManager.h"
#include "Logger.h"

#include <cassert>

void Vignette::Initialize(uint32_t width, uint32_t height)
{
	(void)width;
	(void)height;
	CreateConstantBuffer();
	RegisterPSOs();
}

void Vignette::Finalize()
{
}

std::vector<PostEffect::PassFunc> Vignette::GetPasses()
{
	return { [this](D3D12_CPU_DESCRIPTOR_HANDLE destRTV, uint32_t srcSRVIndex)
		{ Pass(srcSRVIndex, destRTV); } };
}

std::vector<std::vector<PostEffect::PassBarrier>> Vignette::GetBarriers()
{
	return {};
}

void Vignette::Pass(uint32_t srcSRVIndex, D3D12_CPU_DESCRIPTOR_HANDLE destRTV)
{
	auto* commandList = DirectXBase::GetInstance()->GetCommandList();
	auto* srvManager = SrvManager::GetInstance();
	UpdateConstantBuffer();

	const float clearColor[4] = {
		kClearColor_.x, kClearColor_.y, kClearColor_.z, kClearColor_.w };
	commandList->ClearRenderTargetView(destRTV, clearColor, 0, nullptr);
	commandList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

	auto pso = PSOManager::GetInstance()->GetPSOData(
		kPsoName_, PSOManager::BlendMode::None, PSOManager::FillMode::kSolid);
	commandList->SetGraphicsRootSignature(pso.rootSignature.Get());
	commandList->SetPipelineState(pso.pipelineState.Get());
	srvManager->PreDraw();
	commandList->SetGraphicsRootDescriptorTable(
		0, srvManager->GetGPUDescriptorHandle(srcSRVIndex));
	commandList->SetGraphicsRootConstantBufferView(
		1, constantBufferResource_->GetGPUVirtualAddress());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void Vignette::CreateConstantBuffer()
{
	constantBufferResource_ = DirectXBase::GetInstance()->
		CreateConstantBufferResource(sizeof(Param));
	const HRESULT result = constantBufferResource_->Map(
		0, nullptr, reinterpret_cast<void**>(&constantBufferMappedData_));
	assert(SUCCEEDED(result));
	*constantBufferMappedData_ = param_;
}

void Vignette::RegisterPSOs()
{
	auto makeRootSignature = []() -> ComPtr<ID3D12RootSignature>
	{
		D3D12_DESCRIPTOR_RANGE srvRange{};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 1;
		srvRange.BaseShaderRegister = 0;
		srvRange.OffsetInDescriptorsFromTableStart =
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER parameters[2]{};
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
		parameters[0].DescriptorTable.NumDescriptorRanges = 1;
		parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		parameters[1].Descriptor.ShaderRegister = 0;

		D3D12_STATIC_SAMPLER_DESC sampler =
			PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

		D3D12_ROOT_SIGNATURE_DESC description{};
		description.NumParameters = _countof(parameters);
		description.pParameters = parameters;
		description.NumStaticSamplers = 1;
		description.pStaticSamplers = &sampler;
		description.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT result = D3D12SerializeRootSignature(
			&description, D3D_ROOT_SIGNATURE_VERSION_1,
			&signatureBlob, &errorBlob);
		if (FAILED(result))
		{
			if (errorBlob)
			{
				Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			}
			assert(false);
		}

		ComPtr<ID3D12RootSignature> rootSignature;
		result = DirectXBase::GetInstance()->GetDevice()->CreateRootSignature(
			0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature));
		assert(SUCCEEDED(result));
		return rootSignature;
	};

	PSOManager::PSOConfig config{};
	config.vertexShaderPath = L"resources/shaders/PostEffect/Fullscreen.VS.hlsl";
	config.pixelShaderPath = L"resources/shaders/PostEffect/Vignette.PS.hlsl";
	config.rootSignatureGenerator = makeRootSignature;
	config.inputLayoutGenerator = []()
		{ return std::vector<D3D12_INPUT_ELEMENT_DESC>{}; };
	config.depthEnable = false;
	PSOManager::GetInstance()->RegisterPSOConfig(kPsoName_, config);
}

void Vignette::UpdateConstantBuffer()
{
	*constantBufferMappedData_ = param_;
}
