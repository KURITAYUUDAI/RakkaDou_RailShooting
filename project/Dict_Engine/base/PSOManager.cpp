#include "PSOManager.h"
#include "Logger.h"

std::unique_ptr<PSOManager> PSOManager::instance_ = nullptr;

PSOManager* PSOManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = std::make_unique<PSOManager>(ConstructorKey());
	}
	return instance_.get();
}

void PSOManager::Finalize()
{
	psoDatas_.clear();
	rootSignatureDatas_.clear();
	psoConfigs_.clear();
	shaderDatas_.clear();

	instance_.reset();

}

void PSOManager::Initialize()
{
	psoDatas_.clear();
	rootSignatureDatas_.clear();
	psoConfigs_.clear();
	shaderDatas_.clear();
}

void PSOManager::RegisterEnvironmentPSO()
{
	// PSOの設定
	PSOManager::PSOConfig config{};
	config.vertexShaderPath = L"resources/shaders/Environment/Environment.VS.hlsl";
	config.pixelShaderPath = L"resources/shaders/Environment/Environment.PS.hlsl";

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

		D3D12_DESCRIPTOR_RANGE descriptorRangeM[1]{};
		descriptorRangeM[0].BaseShaderRegister = 2; // t1
		descriptorRangeM[0].NumDescriptors = 1;
		descriptorRangeM[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRangeM[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters.resize(9);

		// Enum定義 (可読性のため)
		enum {
			kMaterial, kTransform, kTexture, kLight, kCamera, kEnvironment, kDissolve, kMask, kFog
		};

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	// VertexShaderで使う
		rootParameters[1].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);	// Tableで利用する数

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[3].Descriptor.ShaderRegister = 1;	// レジスタ番号1を使う

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[4].Descriptor.ShaderRegister = 2;	// レジスタ番号2を使う

		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[5].DescriptorTable.pDescriptorRanges = descriptorRangeE;	// Tableの中身の配列を指定
		rootParameters[5].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeE);	// Tableで利用する数

		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[6].Descriptor.ShaderRegister = 3;	// レジスタ番号3を使う

		rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[7].DescriptorTable.pDescriptorRanges = descriptorRangeM;	// Tableの中身の配列を指定
		rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeM);	// Tableで利用する数

		rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[8].Descriptor.ShaderRegister = 4;	// 距離フォグ b4

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

	// 深度設定
	config.depthEnable = true;
	config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	config.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	PSOManager::GetInstance()->RegisterPSOConfig("Environment", config);
}

void PSOManager::RegisterSkinningPSO()
{
	// PSOの設定
	PSOManager::PSOConfig config{};
	config.vertexShaderPath = L"resources/shaders/SkinningObject3d/SkinningObject3d.VS.hlsl";
	config.geometryShaderPath = L"resources/shaders/Object3d/Object3d.GS.hlsl";
	config.pixelShaderPath = L"resources/shaders/Object3d/Object3d.PS.hlsl";

	// RootSignatureの設定
	config.rootSignatureGenerator = [](){
		std::vector<D3D12_ROOT_PARAMETER> rootParameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;
		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler = PSOManager::GetInstance()->GetDefaultStaticSamplerDesc();

		staticSamplerDescs.push_back(sampler);
		D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
		descriptorRange[0].BaseShaderRegister = 0; // t0
		descriptorRange[0].NumDescriptors = 2;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE paletteRange{};
		paletteRange.BaseShaderRegister = 0; // VSのt0
		paletteRange.NumDescriptors = 1;
		paletteRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		paletteRange.OffsetInDescriptorsFromTableStart =D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		
		rootParameters.resize(6);

		// Enum定義 (可読性のため)
		enum {
			kMaterial, kTransform, kTexture, kLight, kCamera, kSkinning
		};

		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	// VertexShaderで使う
		rootParameters[1].Descriptor.ShaderRegister = 0;	// レジスタ番号0とバインド

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);	// Tableで利用する数

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[3].Descriptor.ShaderRegister = 1;	// レジスタ番号1を使う

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
		rootParameters[4].Descriptor.ShaderRegister = 2;	// レジスタ番号2を使う

		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[5].DescriptorTable.pDescriptorRanges = &paletteRange;
		rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;



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
			{ "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "INDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		};

	// 深度設定
	config.depthEnable = true;
	config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	config.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	PSOManager::GetInstance()->RegisterPSOConfig("Skinning", config);
}

D3D12_STATIC_SAMPLER_DESC PSOManager::GetDefaultStaticSamplerDesc()
{
	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	return sampler;
}

const PSOManager::PSOData& PSOManager::GetPSOData(const std::string& name, BlendMode blendMode, FillMode fillMode)
{
	DataKey key{ name, blendMode, fillMode };
	if (psoDatas_.contains(key))
	{
		return psoDatas_[key];
	}
	CreatePipeLineState(name, blendMode, fillMode);
	return psoDatas_[key];
}

void PSOManager::RegisterPSOConfig(const std::string& name, const PSOConfig& config)
{
	if (psoConfigs_.contains(name))
	{
		Logger::Log("This PSOConfig name is already registered for " + name);
		Logger::Log("Priority will be given to those who registered first.");
		return;
	}

	psoConfigs_[name] = config;
}

void PSOManager::CreatePipeLineState(const std::string& name, BlendMode blend, FillMode fill)
{

	const auto& psoConfig = psoConfigs_.at(name);

	// 1. RootSignatureの設定
	if (!rootSignatureDatas_.contains(name))
	{
		assert(psoConfig.rootSignatureGenerator);
		Logger::Log("RootSignatureGenerator is not set for " + name);
		rootSignatureDatas_[name] = psoConfig.rootSignatureGenerator();
	}
	auto rootSignature = rootSignatureDatas_[name];

	// 2. InputLayoutの設定
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	if (psoConfig.inputLayoutGenerator)
	{
		inputElementDescs = psoConfig.inputLayoutGenerator();
	}



	// 3. BlendStateの設定
	D3D12_BLEND_DESC blendDesc = CreateBlendDesc(blend);

	// 4. Shaderをコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> geometryShaderBlob;
	CompileShader(name, vertexShaderBlob, pixelShaderBlob, geometryShaderBlob);
	
	// 5. RasterizerStateの設定
	// ゼロ初期化すると DepthClipEnable まで FALSE になり、far clip より奥の
	// ジオメトリが深度値1.0へ潰れて後続描画に上書きされる。
	// PSOConfig のD3D12既定値を引き継いだ上で、描画モードだけを上書きする。
	D3D12_RASTERIZER_DESC rasterizerDesc = psoConfig.rasterizerDesc;
	rasterizerDesc.CullMode = psoConfig.rasterizerDesc.CullMode;		// 裏面（時計回り）を表示しない
	if (fill == PSOManager::FillMode::kSolid)
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;	// 三角形の中を塗りつぶす
	}
	else
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME; // 三角形の中を塗りつぶさない
	}
	rasterizerDesc.DepthClipEnable = TRUE;			// near/far範囲外を正しくクリップする

	// 6. DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = psoConfig.depthEnable; // Depthの機能を有効化する
	depthStencilDesc.DepthWriteMask = psoConfig.depthWriteMask;	// 書き込まない
	if (psoConfig.depthFunc)
	{
		depthStencilDesc.DepthFunc = psoConfig.depthFunc;	// 比較関数はLessEqual。近ければ描画される
	}
	else
	{
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;	// 比較関数はLessEqual。近ければ描画される
	}

	// 7. PSOを構築
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	{
		// RootSignatureを代入
		graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();	// RootSignature

		// InputLayoutを代入
		graphicsPipelineStateDesc.InputLayout = { inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()) };

		// BlendStateを代入
		graphicsPipelineStateDesc.BlendState = blendDesc;	// BlendState

		// Shaderを代入
		graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
		graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

		if (geometryShaderBlob)
		{
			graphicsPipelineStateDesc.GS = { geometryShaderBlob->GetBufferPointer(), geometryShaderBlob->GetBufferSize() };
		}

		// RasterizerStateを代入
		graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

		// DepthStencilを代入
		graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicsPipelineStateDesc.DSVFormat = psoConfig.depthEnable
			? DXGI_FORMAT_D24_UNORM_S8_UINT
			: DXGI_FORMAT_UNKNOWN;;

		// その他の設定
		graphicsPipelineStateDesc.RTVFormats[0] = DirectXBase::GetInstance()->GetRtvFormat();
		graphicsPipelineStateDesc.NumRenderTargets = 1;
		graphicsPipelineStateDesc.PrimitiveTopologyType = psoConfig.toporogyType;
		graphicsPipelineStateDesc.SampleDesc.Count = 1;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		graphicsPipelineStateDesc.StreamOutput.NumEntries = 0;
		graphicsPipelineStateDesc.StreamOutput.NumStrides = 0;
	}
	
	// 8. PSOを生成
	PSOData psoData;
	psoData.rootSignature = rootSignature;
	HRESULT hr = DirectXBase::GetInstance()->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&psoData.pipelineState));
	assert(SUCCEEDED(hr) && "Failed to create graphics pipeline state");
	
	// 9. キャッシュに保存
	psoDatas_[{name, blend, fill}] = std::move(psoData);

}

D3D12_BLEND_DESC PSOManager::CreateBlendDesc(BlendMode blendMode)
{
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;

	// 共通初期値
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	switch (blendMode) {
	case BlendMode::None:
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		break;
	case BlendMode::Normal:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	case BlendMode::Add:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	case BlendMode::Subtract:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		break;
	case BlendMode::Multiply:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	case BlendMode::Screen:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	}

	return blendDesc;
}

void PSOManager::CompileShader(const std::string& name, Microsoft::WRL::ComPtr<IDxcBlob>& outVS, 
	Microsoft::WRL::ComPtr<IDxcBlob>& outPS, Microsoft::WRL::ComPtr<IDxcBlob>& outGS)
{
	// キャッシュを確認してあれば渡す
	if (shaderDatas_.contains(name))
	{
		outVS = shaderDatas_[name].vertexShader;
		outPS = shaderDatas_[name].pixelShader;
		outGS = shaderDatas_[name].geometryShader;
		return;
	}

	// 無い場合は新規でコンパイル
	psoConfigs_[name];
	
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> geometryShaderBlob;

	vertexShaderBlob = DirectXBase::GetInstance()->CompileShader(psoConfigs_[name].vertexShaderPath, L"vs_6_0");
	pixelShaderBlob = DirectXBase::GetInstance()->CompileShader(psoConfigs_[name].pixelShaderPath, L"ps_6_0");
	
	if (!psoConfigs_[name].geometryShaderPath.empty())
	{
		geometryShaderBlob = DirectXBase::GetInstance()->CompileShader(psoConfigs_[name].geometryShaderPath, L"gs_6_0");
	}

	assert(vertexShaderBlob && pixelShaderBlob);

	shaderDatas_[name] = { vertexShaderBlob, pixelShaderBlob };

	outVS = vertexShaderBlob;
	outPS = pixelShaderBlob;
	outGS = geometryShaderBlob;
}
