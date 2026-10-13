#pragma once

#include "DirectXBase.h"
#include "myMath.h"

#include <cstdint>
#include <memory>

class DistanceFogManager
{
public:
	struct alignas(16) DistanceFogData
	{
		Vector4 color = { 0.005f, 0.008f, 0.015f, 1.0f };
		float startDistance = 40.0f;
		float endDistance = 250.0f;
		int32_t enabled = false;
		float padding = 0.0f;
	};
	static_assert(sizeof(DistanceFogData) == 32);

	static DistanceFogManager* GetInstance();

	class ConstructorKey
	{
	private:
		ConstructorKey() = default;
		friend class DistanceFogManager;
	};

	explicit DistanceFogManager(ConstructorKey) {}

	void Initialize();
	void Finalize();

	void SetCBufferResource(UINT rootParameterIndex) const;

	void SetEnabled(bool enabled);
	void SetStartDistance(float startDistance);
	void SetEndDistance(float endDistance);
	void SetColor(const Vector4& color);

	bool IsEnabled() const;
	float GetStartDistance() const;
	float GetEndDistance() const;
	const Vector4& GetColor() const;

private:
	static constexpr float kMinimumFogRange_ = 0.001f;

	static std::unique_ptr<DistanceFogManager> instance_;

	Microsoft::WRL::ComPtr<ID3D12Resource> constantBufferResource_;
	DistanceFogData* data_ = nullptr;

	~DistanceFogManager() = default;
	DistanceFogManager(const DistanceFogManager&) = delete;
	DistanceFogManager& operator=(const DistanceFogManager&) = delete;
	friend struct std::default_delete<DistanceFogManager>;
};
