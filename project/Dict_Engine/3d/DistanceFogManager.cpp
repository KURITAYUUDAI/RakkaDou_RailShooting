#define NOMINMAX
#include "DistanceFogManager.h"

#include <algorithm>
#include <cassert>

std::unique_ptr<DistanceFogManager> DistanceFogManager::instance_ = nullptr;

DistanceFogManager* DistanceFogManager::GetInstance()
{
	if (!instance_)
	{
		instance_ = std::make_unique<DistanceFogManager>(ConstructorKey());
	}
	return instance_.get();
}

void DistanceFogManager::Initialize()
{
	constantBufferResource_ =
		DirectXBase::GetInstance()->CreateConstantBufferResource(sizeof(DistanceFogData));
	assert(constantBufferResource_);

	HRESULT hr = constantBufferResource_->Map(
		0, nullptr, reinterpret_cast<void**>(&data_));
	assert(SUCCEEDED(hr));

	*data_ = DistanceFogData{};
}

void DistanceFogManager::Finalize()
{
	data_ = nullptr;
	constantBufferResource_.Reset();
	instance_.reset();
}

void DistanceFogManager::SetCBufferResource(UINT rootParameterIndex) const
{
	assert(constantBufferResource_);
	DirectXBase::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(
		rootParameterIndex, constantBufferResource_->GetGPUVirtualAddress());
}

void DistanceFogManager::SetEnabled(bool enabled)
{
	assert(data_);
	data_->enabled = enabled;
}

void DistanceFogManager::SetStartDistance(float startDistance)
{
	assert(data_);
	data_->startDistance = std::max(0.0f, startDistance);
	data_->endDistance =
		std::max(data_->endDistance, data_->startDistance + kMinimumFogRange_);
}

void DistanceFogManager::SetEndDistance(float endDistance)
{
	assert(data_);
	data_->endDistance =
		std::max(endDistance, data_->startDistance + kMinimumFogRange_);
}

void DistanceFogManager::SetColor(const Vector4& color)
{
	assert(data_);
	data_->color = color;
}

bool DistanceFogManager::IsEnabled() const
{
	return data_ && data_->enabled != 0;
}

float DistanceFogManager::GetStartDistance() const
{
	return data_ ? data_->startDistance : 40.0f;
}

float DistanceFogManager::GetEndDistance() const
{
	return data_ ? data_->endDistance : 180.0f;
}

const Vector4& DistanceFogManager::GetColor() const
{
	static const Vector4 kDefaultColor = { 0.005f, 0.008f, 0.015f, 1.0f };
	return data_ ? data_->color : kDefaultColor;
}
