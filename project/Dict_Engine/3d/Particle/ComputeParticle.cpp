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

void ComputeEmitter::Initialize(const std::string& name, const Vector3& translate, const uint32_t& count, const float& frequency)
{
	RegisterEmitPSO();
	CreateEmitterResource();

	emitterSphere_->count = count;
	emitterSphere_->frequency = frequency;
	emitterSphere_->frequencyTime = 0.0f;
	emitterSphere_->translate = translate;
	emitterSphere_->radius = 1.0f;
	emitterSphere_->emit = 0;
}

void ComputeEmitter::Update(const float& deltaTime)
{
	emitterSphere_->frequencyTime += deltaTime; // deltaTimeを加算
	// 射出間隔を上回ったら射出許可を出して時間を調整
	if (emitterSphere_->frequency <= emitterSphere_->frequencyTime)
	{
		emitterSphere_->frequencyTime -= emitterSphere_->frequency;
		emitterSphere_->emit = 1;
	} 
	else // 射出間隔を上回っていないので、射出許可は出せない
	{
		emitterSphere_->emit = 0;
	}

}

void ComputeEmitter::Dispatch(uint32_t particleCSUavIndex)
{
	auto psoSet = PSOManager::GetInstance()->GetComputePSOData(kPsoName);
	auto commandList = DirectXBase::GetInstance()->GetCommandList();

	SrvManager::GetInstance()->PreDraw();

	commandList->SetComputeRootSignature(psoSet.rootSignature.Get());
	commandList->SetPipelineState(psoSet.pipelineState.Get());
	commandList->SetComputeRootDescriptorTable(0, SrvManager::GetInstance()->GetGPUDescriptorHandle(particleCSUavIndex));
	commandList->SetComputeRootConstantBufferView(1, emitterResource_->GetGPUVirtualAddress());
	commandList->Dispatch(1, 1, 1);

	// Compute書き込み後、ピクセルシェーダーとして読める状態へ
	TransitionEmitterResource(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void ComputeEmitter::RegisterEmitPSO()
{
	PSOManager::ComputePSOConfig config{};
	config.computeShaderPath = L"resources/shaders/Particle/EmitParticle.CS.hlsl";

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

		rootParameters.resize(2);
		enum
		{
			kParticleUav, kEmitter
		};

		rootParameters[kParticleUav].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// DescriptorTableを使う
		rootParameters[kParticleUav].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[kParticleUav].DescriptorTable.pDescriptorRanges = &descriptorRange;	// Tableの中身の配列を指定
		rootParameters[kParticleUav].DescriptorTable.NumDescriptorRanges = 1;	// Tableで利用する数

		rootParameters[kEmitter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[kEmitter].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 全てのシェーダーで使う
		rootParameters[kEmitter].Descriptor.ShaderRegister = 0;	// レジスタ番号0を使う

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

	PSOManager::GetInstance()->RegisterComputePSOConfig("EmitParticle", config);
}

void ComputeEmitter::CreateEmitterResource()
{
	emitterResource_ = DirectXBase::GetInstance()->CreateConstantBufferResource(sizeof(EmitterSphere));
	emitterResource_->Map(0, nullptr, reinterpret_cast<void**>(&emitterSphere_));
}

void ComputeEmitter::TransitionEmitterResource(D3D12_RESOURCE_STATES nextState)
{
	if (emitterState_ == nextState)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	barrier.Transition.pResource = emitterResource_.Get();
	barrier.Transition.StateBefore = emitterState_;
	barrier.Transition.StateAfter = nextState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	DirectXBase::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);

	emitterState_ = nextState;
}
