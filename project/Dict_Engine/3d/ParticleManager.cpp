#define NOMINMAX
#include "ParticleManager.h"
#include "Logger.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "random/SeedManager.h"
#include "ModelManager.h"
#include "Model.h"
#include "CameraManager.h"
#include "Camera.h"
#include "ResourcePath.h"
#include "time/DeltaTimeManager.h"

std::unique_ptr<ParticleManager> ParticleManager::instance_ = nullptr;

void ParticleManager::Initialize(DirectXBase* dxBase)
{
	dxBase_ = dxBase;

	srvManager_ = SrvManager::GetInstance();

	// PSOの設定
	PSOManager::PSOConfig config{};
	config.vertexShaderPath = L"Resources/Shaders/Particle/Particle.VS.hlsl";
	config.pixelShaderPath = L"Resources/Shaders/Particle/Particle.PS.hlsl";

	// RootSignatureの設定
	config.rootSignatureGenerator = []
		{
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;
		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	
		staticSamplerDescs.push_back(sampler);
		D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
		descriptorRange[0].BaseShaderRegister = 0; // t0
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters.resize(4);

		// Enum定義 (可読性のため)
		enum {
			kMaterial, kTransform, kTexture, kCamera,
		};

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	// VertexShaderで使う
		rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);	// Tableで利用する数

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);	// Tableで利用する数

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[3].Descriptor.ShaderRegister = 1;	// レジスタ番号1を使う



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
		return std::vector<D3D12_INPUT_ELEMENT_DESC>
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		};

	// ラスタライザ設定
	config.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	// 深度設定
	config.depthEnable = true;
	config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	config.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	PSOManager::GetInstance()->RegisterPSOConfig(psoName_, config);

	//CreateGraphicsPipelineState();
}

ParticleManager* ParticleManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<ParticleManager>(ConstructorKey());
	}
	return instance_.get();
}

void ParticleManager::Finalize()
{
	instance_.reset();
}

ParticleParentHandle ParticleManager::RegisterParticleParent()
{
	uint32_t index = 0;
	if (!freeParticleParentIndices_.empty())
	{
		index = freeParticleParentIndices_.back();
		freeParticleParentIndices_.pop_back();
	}
	else
	{
		index = static_cast<uint32_t>(particleParentSlots_.size());
		particleParentSlots_.push_back({});
	}

	ParticleParentSlot& slot = particleParentSlots_[index];
	slot.worldMatrix = MakeIdentity4x4();
	slot.active = true;

	return {
		.index = index,
		.generation = slot.generation,
	};
}

void ParticleManager::UpdateParticleParent(
	ParticleParentHandle handle, const Matrix4x4& worldMatrix)
{
	if (handle.index >= particleParentSlots_.size())
	{
		return;
	}

	ParticleParentSlot& slot = particleParentSlots_[handle.index];
	if (!slot.active || slot.generation != handle.generation)
	{
		return;
	}

	slot.worldMatrix = worldMatrix;
}

void ParticleManager::UnregisterParticleParent(ParticleParentHandle handle)
{
	if (handle.index >= particleParentSlots_.size())
	{
		return;
	}

	ParticleParentSlot& slot = particleParentSlots_[handle.index];
	if (!slot.active || slot.generation != handle.generation)
	{
		return;
	}

	slot.active = false;
	slot.worldMatrix = MakeIdentity4x4();
	++slot.generation;
	if (slot.generation == 0)
	{
		slot.generation = 1;
	}
	freeParticleParentIndices_.push_back(handle.index);
}

const Matrix4x4* ParticleManager::ResolveParticleParent(
	ParticleParentHandle handle) const
{
	if (handle.index >= particleParentSlots_.size())
	{
		return nullptr;
	}

	const ParticleParentSlot& slot = particleParentSlots_[handle.index];
	if (!slot.active || slot.generation != handle.generation)
	{
		return nullptr;
	}

	return &slot.worldMatrix;
}

void ParticleManager::Update(const float& deltaTime)
{
	deltaTime_ = DeltaTimeManager::GetInstance()->GetDeltaTime(DeltaTimeGroup::Effect);

	viewProjectionMatrix_ = CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix();

	for (auto& particleGroup : particleGroups_)
	{
		particleGroup.second.instanceNum = 0;

		for (auto it = particleGroup.second.particles.begin(); it != particleGroup.second.particles.end();)
		{
			Particle& particle = *it;

			if (particle.lifeTime <= particle.currentTime)
			{
				it = particleGroup.second.particles.erase(it);
				continue;
			}

			const Matrix4x4* parentWorldMatrix = nullptr;
			if (particle.parentHandle.IsValid())
			{
				parentWorldMatrix = ResolveParticleParent(particle.parentHandle);
				if (!parentWorldMatrix)
				{
					it = particleGroup.second.particles.erase(it);
					continue;
				}
			}

			const Vector3 worldPositionBeforeMovement = parentWorldMatrix ?
				TransformPosition(particle.transform.translate, *parentWorldMatrix) :
				particle.transform.translate;

			for (const AccelerationField& accelerationField : accelerationFields_)
			{
				if (IsCollision(accelerationField.area, worldPositionBeforeMovement)
					&& particleGroup.second.isMoveAccelerationField)
				{
					const Vector3 localAcceleration = parentWorldMatrix ?
						TransformNormal(accelerationField.acceleration,
							Inverse(*parentWorldMatrix)) :
						accelerationField.acceleration;
					particle.velocity += localAcceleration * deltaTime_;
				}
			}

			particle.transform.translate += particle.velocity * deltaTime_;

			if (particleGroup.second.isScaleOverLife)
			{
				const float progress = std::clamp(
					particle.currentTime / std::max(particle.lifeTime, 0.001f), 0.0f, 1.0f);
				const float inverseProgress = 1.0f - progress;
				const float easedProgress = 1.0f - inverseProgress * inverseProgress * inverseProgress;
				const Vector3& startScale = particleGroup.second.startScale;
				const Vector3& endScale = particleGroup.second.endScale;
				particle.transform.scale =
				{
					startScale.x + (endScale.x - startScale.x) * easedProgress,
					startScale.y + (endScale.y - startScale.y) * easedProgress,
					startScale.z + (endScale.z - startScale.z) * easedProgress,
				};
			}

			particle.color.w = particle.initialAlpha *
				(1.0f - (particle.currentTime / particle.lifeTime));

			particle.currentTime += deltaTime_;

			Matrix4x4 worldMatrix;

			if (particleGroup.second.isBillboard)
			{
				const Vector3 worldPosition = parentWorldMatrix ?
					TransformPosition(particle.transform.translate, *parentWorldMatrix) :
					particle.transform.translate;
				worldMatrix = CameraManager::GetInstance()->GetMainCamera()->GetBillboardWorldMatrix(
					particle.transform.scale,
					particle.transform.rotate,
					worldPosition);
			}
			else
			{
				worldMatrix = MakeAffineMatrix(particle.transform.scale,
					particle.transform.rotate, particle.transform.translate);
				if (parentWorldMatrix)
				{
					worldMatrix *= *parentWorldMatrix;
				}
			}
			
			/*particle.uvTransform.translate.x += 0.01f;
			if (particle.uvTransform.translate.x > 1.0f)
			{
				particle.uvTransform.translate.x -= 1.0f;
			}*/

			Matrix4x4 uvTransformMatrix;
			uvTransformMatrix = MakeAffineMatrix(
				particle.uvTransform.scale, particle.uvTransform.rotate, particle.uvTransform.translate);

			if (particleGroup.second.instanceNum < particleGroup.second.maxInstanceNum)
			{
				Matrix4x4 worldViewProjectionMatrix;
				/*if (defaultCamera_)
				{
					const Matrix4x4& viewProjectionMatrix = defaultCamera_->GetViewProjectionMatrix();
					worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
				} 
				else
				{
					worldViewProjectionMatrix = worldMatrix;
				}*/
				const Matrix4x4& viewProjectionMatrix = CameraManager::GetInstance()->GetMainCamera()->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);

				particleGroup.second.instancingData[particleGroup.second.instanceNum].WVP = worldViewProjectionMatrix;
				particleGroup.second.instancingData[particleGroup.second.instanceNum].World = worldMatrix;
				particleGroup.second.instancingData[particleGroup.second.instanceNum].color = particle.color;
				particleGroup.second.instancingData[particleGroup.second.instanceNum].uvTransform = uvTransformMatrix;

				++particleGroup.second.instanceNum;
			}

			++it;
		}
	}
}

void ParticleManager::Draw()
{
	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	dxBase_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& particleGroup : particleGroups_)
	{
		auto psoSet = PSOManager::GetInstance()->GetPSOData(
			particleGroup.second.psoName_, particleGroup.second.blendMode_, particleGroup.second.fillMode_);

		// RootSignatureを設定。PSOに設定しているけど別途設定が必要
		dxBase_->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());
		dxBase_->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());	// PS0を設定

		dxBase_->GetCommandList()->SetGraphicsRootDescriptorTable(1, 
			srvManager_->GetGPUDescriptorHandle(particleGroup.second.instancingSrvIndex));

	
		// 3Dモデルが割り当てられていれば描画する
		if (particleGroup.second.model && particleGroup.second.material)
		{
			particleGroup.second.material->Draw(0, 2);
			particleGroup.second.model->Draw(particleGroup.second.instanceNum);
		}
	}

}

void ParticleManager::Reset()
{
	particleGroups_.clear();
	accelerationFields_.clear();
}

void ParticleManager::DrawingCommon()
{
	// RootSignatureを設定。PSOに設定しているけど別途設定が必要
	dxBase_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
	dxBase_->GetCommandList()->SetPipelineState(graphicsPipeLineState_.Get());	// PS0を設定

	// 形状を設定。PS0に設定しているものとはまた別。同じものを設定すると考えておけば良い
	dxBase_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath)
{
	if (particleGroups_.contains(name))
	{
		return;
	}

	// パーティクルグループを追加
	ParticleGroup& particleGroup = particleGroups_[name];
	particleGroup.maxInstanceNum = 30;
	
	// テクスチャをロード
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	particleGroup.material = std::make_unique<MaterialInstance>();
	particleGroup.material->Initialize();
	particleGroup.material->SetTexture(textureFilePath);


	// Instancing用のTransformationMatrixResourceを作成する。
	particleGroup.instancingResource
		= dxBase_->CreateBufferResource(sizeof(ParticleForGPU) * particleGroup.maxInstanceNum);
	// InstancingResourceにデータを書き込むためのアドレスを取得してInstancingDataに割り当てる
	particleGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&particleGroup.instancingData));

	// インスタンスごとの座標変換行列データを書き込む
	for (size_t i = 0; i < particleGroup.maxInstanceNum; ++i)
	{
		particleGroup.instancingData[i].World = MakeIdentity4x4();
		particleGroup.instancingData[i].WVP = MakeIdentity4x4();
		particleGroup.instancingData[i].color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		particleGroup.instancingData[i].uvTransform = MakeIdentity4x4();
	}

	// SRVを作成
	particleGroup.instancingSrvIndex = SrvManager::GetInstance()->AllocateIndex();
	SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
		particleGroup.instancingSrvIndex,
		particleGroup.instancingResource.Get(),
		particleGroup.maxInstanceNum, sizeof(ParticleForGPU)
	);

	particleGroup.isMoveAccelerationField = true;

	// PSOは一旦デフォルトで
	particleGroup.psoName_ = psoName_;
	particleGroup.blendMode_ = PSOManager::BlendMode::Add;
	particleGroup.fillMode_ = PSOManager::FillMode::kSolid;
}

void ParticleManager::RandomEmit(const std::string name, const Vector3& position,
	uint32_t count, ParticleParentHandle parentHandle)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	for (uint32_t index = 0; index < count; ++index)
	{
		Particle particle;
		const ParticleGroup& particleGroup = particleGroups_[name];

		particle.transform =
		{
			particleGroup.isScaleOverLife
				? particleGroup.startScale
				: Vector3{ 1.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f },
			position +
			Vector3{
				SeedManager::GetInstance()->GenerateFloat(-1.0f, 1.0f),
				SeedManager::GetInstance()->GenerateFloat(-1.0f, 1.0f),
				SeedManager::GetInstance()->GenerateFloat(-1.0f, 1.0f),
			}
		};

		particle.velocity =
		{
			/*SeedManager::GetInstance()->GenerateFloat(-1.0f, 1.0f),
			SeedManager::GetInstance()->GenerateFloat(-1.0f, 1.0f),
			SeedManager::GetInstance()->GenerateFloat(-1.0f, 1.0f),*/
		};

		particle.color =
		{
			SeedManager::GetInstance()->GenerateFloat(0.0f, 1.0f),
			SeedManager::GetInstance()->GenerateFloat(0.0f, 1.0f),
			SeedManager::GetInstance()->GenerateFloat(0.0f, 1.0f),
			1.0f
		};

		particle.lifeTime = particleGroup.useFixedLifeTime
			? particleGroup.fixedLifeTime
			: SeedManager::GetInstance()->GenerateFloat(1.0f, 3.0f);

		particle.currentTime = 0.0f;
		particle.parentHandle = parentHandle;

		particleGroups_[name].particles.push_back(particle);
	}
}

void ParticleManager::EmitSlash(const std::string name, const Vector3& position,
	uint32_t count, ParticleParentHandle parentHandle)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	for (uint32_t index = 0; index < count; ++index)
	{
		Particle particle;
		particle.transform.scale = { 0.05f, SeedManager::GetInstance()->GenerateFloat(0.4f, 1.5f), 1.0f };
		particle.transform.rotate = { 0.0f, 0.0f, SeedManager::GetInstance()->GenerateFloat(-pi, pi) };
		particle.transform.translate = position;
		particle.velocity = { 0.0f, 0.0f, 0.0f };
		particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		particle.lifeTime = 1.0f;
		particle.currentTime = 0.0f;
		particle.parentHandle = parentHandle;

		particleGroups_[name].particles.push_back(particle);
	}
}

void ParticleManager::Emit(const std::string name, const Vector3& position,
	uint32_t count, ParticleParentHandle parentHandle)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	for (uint32_t index = 0; index < count; ++index)
	{
		Particle particle;
		const ParticleGroup& particleGroup = particleGroups_[name];
		particle.transform.scale = particleGroup.isScaleOverLife
			? particleGroup.startScale
			: Vector3{ 1.0f, 1.0f, 1.0f };
		particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
		particle.transform.translate = position;
		particle.velocity = { 0.0f, 0.0f, 0.0f };
		particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		particle.lifeTime = particleGroup.useFixedLifeTime
			? particleGroup.fixedLifeTime
			: 1.0f;
		particle.currentTime = 0.0f;
		particle.parentHandle = parentHandle;

		particleGroups_[name].particles.push_back(particle);
	}
}

void ParticleManager::EmitParticle(
	const std::string& name, const EulerTransform& transform,
	const Vector3& velocity, const Vector4& color, float lifeTime,
	ParticleParentHandle parentHandle)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	ParticleGroup& particleGroup = particleGroups_[name];
	if (particleGroup.particles.size() >= particleGroup.maxInstanceNum)
	{
		return;
	}

	Particle particle;
	particle.transform = transform;
	particle.velocity = velocity;
	particle.color = color;
	particle.initialAlpha = std::clamp(color.w, 0.0f, 1.0f);
	particle.color.w = particle.initialAlpha;
	particle.lifeTime = std::max(lifeTime, 0.001f);
	particle.currentTime = 0.0f;
	particle.parentHandle = parentHandle;
	particleGroup.particles.push_back(particle);
}

void ParticleManager::CreateAccelerationField(const Vector3& acceleration, const AABB& area)
{
	AccelerationField accelerationField;
	accelerationField.acceleration = acceleration;
	accelerationField.area = area;

	accelerationFields_.push_back(accelerationField);
}

void ParticleManager::SetModel(const std::string& filePath)
{
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}

void ParticleManager::SetModel(const std::string& name, std::string filePath)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	particleGroups_[name].model = ModelManager::GetInstance()->FindModel(filePath);
	//particleGroups_[name].material->SetMaterialAsset(particleGroups_[name].model->GetMaterialAsset(0));
}

void ParticleManager::SetIsMoveAccelerationField(const std::string& name, bool isMove)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	particleGroups_[name].isMoveAccelerationField = isMove;
}

void ParticleManager::SetIsBillboard(const std::string& name, bool isBillboard)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	particleGroups_[name].isBillboard = isBillboard;
}

void ParticleManager::SetScaleOverLife(
	const std::string& name, const Vector3& startScale, const Vector3& endScale)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	ParticleGroup& particleGroup = particleGroups_[name];
	particleGroup.isScaleOverLife = true;
	particleGroup.startScale = startScale;
	particleGroup.endScale = endScale;
}

void ParticleManager::SetParticleLifeTime(const std::string& name, float lifeTime)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	ParticleGroup& particleGroup = particleGroups_[name];
	particleGroup.useFixedLifeTime = true;
	particleGroup.fixedLifeTime = std::max(lifeTime, 0.001f);
}

void ParticleManager::SetBlendMode(
	const std::string& name, PSOManager::BlendMode blendMode)
{
	if (!particleGroups_.contains(name))
	{
		assert(false);
		return;
	}

	particleGroups_[name].blendMode_ = blendMode;
}

const std::list<std::string> ParticleManager::GetParticleGroupName()
{
	std::list<std::string> particleGroupName;

	for (auto& particleGroup : particleGroups_)
	{
		particleGroupName.push_back(particleGroup.first);
	}

	return particleGroupName;
}

//void ParticleManager::CreateRootSignature(ID3DBlob* signatureBlob)
//{
//	HRESULT hr;
//	hr = dxBase_->GetDevice()->CreateRootSignature(0,
//		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
//		IID_PPV_ARGS(&rootSignature_));
//	assert(SUCCEEDED(hr));
//}
//
//void ParticleManager::CreateGraphicsPipelineState()
//{
//	HRESULT hr;
//
//	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
//	descriptorRangeForInstancing[0].BaseShaderRegister = 0; // 0から始まる
//	descriptorRangeForInstancing[0].NumDescriptors = 1;	// 数は一つ
//	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
//	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算
//
//	// RootSignature作成
//	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
//	descriptionRootSignature.Flags =
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
//
//	// RootParameter作成。複数設定できるので配列。PixelShaderのMaterialとVertexShaderのTransform
//	D3D12_ROOT_PARAMETER rootParameters[4] = {};
//	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
//	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
//	rootParameters[0].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド
//
//	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
//	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	// VertexShaderで使う
//	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;	// Tableの中身の配列を指定
//	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);	// Tableで利用する数
//
//	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
//	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
//	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;	// Tableの中身の配列を指定
//	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);	// Tableで利用する数
//
//	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
//	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
//	rootParameters[3].Descriptor.ShaderRegister = 1;	// レジスタ番号1を使う
//
//	descriptionRootSignature.pParameters = rootParameters;	// ルートパラメータ配列へのポインタ
//	descriptionRootSignature.NumParameters = _countof(rootParameters);	// 配列の長さ
//
//
//	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
//	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	// バイリニアフィルタ
//	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	// 0～1の範囲外をリピート
//	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	// 比較しない
//	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;	// ありったけのMipMapを使う
//	staticSamplers[0].ShaderRegister = 0;	// レジスタ番号0を使う
//	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
//	descriptionRootSignature.pStaticSamplers = staticSamplers;
//	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
//
//	// シリアライズしてバイナリにする
//	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
//	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
//	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
//		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
//	if (FAILED(hr))
//	{
//		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
//		assert(false);
//	}
//
//	/// 02_00
//
//	// バイナリをもとに生成
//	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
//	hr = dxBase_->GetDevice()->CreateRootSignature(0,
//		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
//		IID_PPV_ARGS(&rootSignature));
//	assert(SUCCEEDED(hr));
//
//	// InputLayout
//	/*D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
//	inputElementDescs[0].SemanticName = "POSITION";
//	inputElementDescs[0].SemanticIndex = 0;
//	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
//	inputLayoutDesc.pInputElementDescs = inputElementDescs;
//	inputLayoutDesc.NumElements = _countof(inputElementDescs);*/
//
//	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
//	inputElementDescs[0].SemanticName = "POSITION";
//	inputElementDescs[0].SemanticIndex = 0;
//	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
//	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//	inputElementDescs[1].SemanticName = "TEXCOORD";
//	inputElementDescs[1].SemanticIndex = 0;
//	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
//	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//	inputElementDescs[2].SemanticName = "NORMAL";
//	inputElementDescs[2].SemanticIndex = 0;
//	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
//	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
//	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
//	inputLayoutDesc.pInputElementDescs = inputElementDescs;
//	inputLayoutDesc.NumElements = _countof(inputElementDescs);
//
//	// BlendStateの設定
//	D3D12_BLEND_DESC blendDesc{};
//	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//	blendDesc.RenderTarget[0].BlendEnable = true;
//	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
//	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//
//	// RasiterzerStateの設定
//	D3D12_RASTERIZER_DESC rasterizerDesc{};
//	// 裏面（時計回り）を表示しない
//	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
//	// 三角形の中を塗りつぶす
//	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
//
//	// Shaderをコンパイルする
//	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxBase_->CompileShader(
//		L"resources/shaders/Particle/Particle.VS.hlsl", L"vs_6_0");
//	assert(vertexShaderBlob != nullptr);
//
//	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxBase_->CompileShader(
//		L"resources/shaders/Particle/Particle.PS.hlsl", L"ps_6_0");
//	assert(pixelShaderBlob != nullptr);
//
//
//	CreateRootSignature(signatureBlob.Get());
//
//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
//
//	// DepthStencilStateの設定
//	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
//
//	// Depthの機能を有効化する
//	depthStencilDesc.DepthEnable = true;
//	// 書き込まない
//	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
//	// 比較関数はLessEqual。つまり、近ければ描画される
//	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//
//	// DepthStencilの設定
//	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
//	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//
//	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();	// RootSignature
//	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;	// InputLayout
//	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
//	vertexShaderBlob->GetBufferSize() };	// VertexShader
//	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
//	pixelShaderBlob->GetBufferSize() };		// PixelShader
//	graphicsPipelineStateDesc.BlendState = blendDesc;	// BlendState
//	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;	// RasterizerState
//
//	// 書き込むRTVの情報
//	graphicsPipelineStateDesc.NumRenderTargets = 1;
//	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//
//	// 利用するトポロジ（形状）のタイプ。三角形
//	graphicsPipelineStateDesc.PrimitiveTopologyType =
//		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//
//	// どのように画面に色を打ち込むかの設定（気にしなくて良い）
//	graphicsPipelineStateDesc.SampleDesc.Count = 1;
//	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//
//	// 実際に生成
//	hr = dxBase_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
//		IID_PPV_ARGS(&graphicsPipeLineState_));
//	assert(SUCCEEDED(hr));
//
//}


