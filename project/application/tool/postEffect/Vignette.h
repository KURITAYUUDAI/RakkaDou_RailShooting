#pragma once
#include "PostEffect.h"

class Vignette : public PostEffect
{
public:
	struct Param
	{
		float threshold = 0.35f;
		float softness = 0.45f;
		float intensity = 0.0f;
		float padding = 0.0f;
		Vector4 darkColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	void Initialize(uint32_t width, uint32_t height) override;
	void Finalize() override;
	std::vector<PassFunc> GetPasses() override;
	std::vector<std::vector<PassBarrier>> GetBarriers() override;

	void SetThreshold(float threshold) { param_.threshold = threshold; }
	void SetSoftness(float softness) { param_.softness = softness; }
	void SetIntensity(float intensity) { param_.intensity = intensity; }
	void SetDarkColor(const Vector4& color) { param_.darkColor = color; }

private:
	void Pass(uint32_t srcSRVIndex, D3D12_CPU_DESCRIPTOR_HANDLE destRTV);
	void CreateConstantBuffer();
	void RegisterPSOs();
	void UpdateConstantBuffer();

	ComPtr<ID3D12Resource> constantBufferResource_;
	Param* constantBufferMappedData_ = nullptr;
	Param param_{};
	const std::string kPsoName_ = "Vignette";
};
