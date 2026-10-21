#pragma once
#include "DirectXBase.h"
#include "myMath.h"

#include <memory>
#include <unordered_map>

class PSOManager
{
public:

	// シングルトンインスタンスの取得
	static PSOManager* GetInstance();
	// 終了
	void Finalize();

	// コンストラクタに渡すための鍵
	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class PSOManager;
	};

	// PassKeyを受け取るコンストラクタ
	explicit PSOManager(ConstructorKey){}

private:

	// unique_ptr の型定義に Deleter を入れることでdeleteが可能になる
	static std::unique_ptr<PSOManager> instance_;

	~PSOManager() = default;
	PSOManager(PSOManager&) = delete;
	PSOManager& operator=(PSOManager&) = delete;

	friend struct std::default_delete<PSOManager>;


public:

	struct ShaderData
	{
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShader;
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShader;
		Microsoft::WRL::ComPtr<IDxcBlob> geometryShader;
	};

	struct InputElement
	{
		
	};

	struct PSOConfig
	{
		std::wstring vertexShaderPath;
		std::wstring pixelShaderPath;
		std::wstring geometryShaderPath;

		D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		using RootSignatureGenerator = std::function<Microsoft::WRL::ComPtr<ID3D12RootSignature>()>;
		RootSignatureGenerator rootSignatureGenerator;

		using InputLayoutGenerator = std::function<std::vector<D3D12_INPUT_ELEMENT_DESC>()>;
		InputLayoutGenerator inputLayoutGenerator;

		D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		bool depthEnable = true;
		D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE toporogyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	};

	enum class BlendMode { None, Normal, Add, Subtract, Multiply, Screen };
	enum class FillMode { kSolid, kWireFrame };

	struct ComputePSOConfig
	{
		std::wstring computeShaderPath;
		using RootSignatureGenerator = std::function<Microsoft::WRL::ComPtr<ID3D12RootSignature>()>;
		RootSignatureGenerator rootSignatureGenerator;
	};

	struct PSOData
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	};

public:

	void Initialize();

public:

	void RegisterEnvironmentPSO();

	void RegisterSkinningPSO();

	void RegisterSkinningComputePSO();

public: // 外部入出力

	D3D12_STATIC_SAMPLER_DESC GetDefaultStaticSamplerDesc();

	const PSOData& GetPSOData(const std::string& name, BlendMode blendMode, FillMode fillMode);
	void RegisterPSOConfig(const std::string& name, const PSOConfig& config);

	const PSOData& GetComputePSOData(const std::string& name);
	void RegisterComputePSOConfig(const std::string& name, const ComputePSOConfig& config);


private:

	struct DataKey
	{
		std::string name;
		BlendMode blendMode;
		FillMode fillMode;
		bool operator==(const DataKey& other) const
		{
			return name == other.name && blendMode == other.blendMode && fillMode == other.fillMode;
		}
	};

	struct DataKeyHash
	{
		std::size_t operator()(const DataKey& key) const
		{
			std::size_t h1 = std::hash<std::string>{}(key.name);
			std::size_t h2 = std::hash<int>{}(static_cast<int>(key.blendMode));
			std::size_t h3 = std::hash<int>{}(static_cast<int>(key.fillMode));
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};


private:

	void CreatePipeLineState(const std::string& name, BlendMode blend, FillMode fill);
	D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode);
	void CompileShader(const std::string& name, Microsoft::WRL::ComPtr<IDxcBlob>& outVS,
		Microsoft::WRL::ComPtr<IDxcBlob>& outPS, Microsoft::WRL::ComPtr<IDxcBlob>& outGS);

	void CreateComputePipeLineState(const std::string& name);
	void CompileComputeShader(const std::string& name, Microsoft::WRL::ComPtr<IDxcBlob>& outCS);

private:

	std::unordered_map<std::string, ShaderData> shaderDatas_;

	std::unordered_map<std::string, PSOConfig> psoConfigs_;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatureDatas_;

	std::unordered_map<DataKey, PSOData, DataKeyHash> psoDatas_;

	std::unordered_map<std::string, ComputePSOConfig> computePSOConfigs_;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> computeRootSignatureDatas_;

	std::unordered_map<std::string, PSOData> computePSODatas_;
};

