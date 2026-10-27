#pragma once
#include "PostEffect.h"

class HSVAdjust : public PostEffect
{
public:
	struct Param
	{
		float hueShift = 0.0f;
		float saturation = 1.0f;
		float value = 1.0f;
		float intensity = 1.0f;
	};

	void Initialize(uint32_t width, uint32_t height) override;
	void Finalize() override;
	std::vector<PassFunc> GetPasses() override;
	std::vector<std::vector<PassBarrier>> GetBarriers() override;

	void SetHueShift(float hueShift) { param_.hueShift = hueShift; }
	void SetSaturation(float saturation) { param_.saturation = saturation; }
	void SetValue(float value) { param_.value = value; }
	void SetIntensity(float intensity) { param_.intensity = intensity; }

private:
	void Pass(uint32_t srcSRVIndex, D3D12_CPU_DESCRIPTOR_HANDLE destRTV);
	void CreateConstantBuffer();
	void RegisterPSOs();
	void UpdateConstantBuffer();

	ComPtr<ID3D12Resource> constantBufferResource_;
	Param* constantBufferMappedData_ = nullptr;
	Param param_{};
	const std::string kPsoName_ = "HSVAdjust";
};
