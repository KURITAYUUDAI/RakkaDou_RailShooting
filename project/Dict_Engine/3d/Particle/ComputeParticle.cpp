#include "ComputeParticle.h"
#include "SrvManager.h"
#include "Logger.h"

void ComputeParticle::Initialize(uint32_t maxParticleCount)
{
	maxParticleCount_ = maxParticleCount;

	RegisterInitializePSO();
	CreateParticleCSResource();
	DispatchInitialize();
}

void ComputeParticle::DispatchInitialize()
{
	auto psoSet = PSOManager::GetInstance()->GetComputePSOData(kInitializePsoName);
	auto commandList = DirectXBase::GetInstance()->GetCommandList();

	SrvManager::GetInstance()->PreDraw();

	// COMMONまたは前フレームのVBV状態からUAVへ
	TransitionParticleResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootSignature(psoSet.rootSignature.Get());
	commandList->SetPipelineState(psoSet.pipelineState.Get());
	commandList->SetComputeRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUDescriptorHandle(particleCSUavIndex_));
	commandList->Dispatch(UINT(maxParticleCount_ + 1023) / 1024, 1, 1);

	// Compute書き込み後、ピクセルシェーダーとして読める状態へ
	TransitionParticleResource(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void ComputeParticle::RegisterInitializePSO()
{
	PSOManager::ComputePSOConfig config{};
	config.computeShaderPath = L"resources/shaders/Particle/InitializeParticle.CS.hlsl";
	
	config.rootSignatureGenerator = [](){
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;
		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();

		D3D12_DESCRIPTOR_RANGE descriptorRange{};
		descriptorRange.BaseShaderRegister = 0; // u0
		descriptorRange.NumDescriptors = 1;
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		
		rootParameters.resize(1);
		enum 
		{
			kParticleUav
		};

		rootParameters[kParticleUav].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// DescriptorTableを使う
		rootParameters[kParticleUav].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[kParticleUav].DescriptorTable.pDescriptorRanges = &descriptorRange;	// Tableの中身の配列を指定
		rootParameters[kParticleUav].DescriptorTable.NumDescriptorRanges = 1;	// Tableで利用する数

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
		if (FAILED(hr)) 
		{
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			assert(false);
		}

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		hr = DirectXBase::GetInstance()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		assert(SUCCEEDED(hr));



		return rootSignature;
	};

	PSOManager::GetInstance()->RegisterComputePSOConfig("InitializeParticle", config);
}

void ComputeParticle::CreateParticleCSResource()
{
	UINT sizeInBytes = static_cast<UINT>(sizeof(GPUParticle) * maxParticleCount_);

	particleCSResource_ = DirectXBase::GetInstance()->CreateUnorderedResource(sizeInBytes);

	particleCSUavIndex_ = SrvManager::GetInstance()->AllocateIndex();

	SrvManager::GetInstance()->CreateUAVforStructuredBuffer(
		particleCSUavIndex_,
		particleCSResource_.Get(),
		maxParticleCount_,
		sizeof(GPUParticle)
	);

	particleCSSrvIndex_ = SrvManager::GetInstance()->AllocateIndex();

	SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
		particleCSSrvIndex_,
		particleCSResource_.Get(),
		maxParticleCount_,
		sizeof(GPUParticle)
	);
}

void ComputeParticle::TransitionParticleResource(D3D12_RESOURCE_STATES nextState)
{
	if (particleState_ == nextState)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	barrier.Transition.pResource = particleCSResource_.Get();
	barrier.Transition.StateBefore = particleState_;
	barrier.Transition.StateAfter = nextState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	DirectXBase::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);

	particleState_ = nextState;
}
