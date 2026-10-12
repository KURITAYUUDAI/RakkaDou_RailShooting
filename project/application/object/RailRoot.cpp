#define NOMINMAX
#include "RailRoot.h"

#include "DebugDrawManager.h"
#include "ImGuiManager.h"

#include <algorithm>
#include <cmath>

void RailRoot::Initialize()
{
	controlPoints_ =
	{
		{-20.0f, 0.0f,  0.0f},
		{-15.0f,  0.0f,  15.0f},
		{ 0.0f,  0.0f, 20.0f},
		{ 15.0f,  0.0f,  15.0f},
		{20.0f,  0.0f,  0.0f},
		{ 15.0f,  0.0f, -15.0f},
		{ 0.0f,  0.0f,-20.0f},
		{-15.0f,  0.0f, -15.0f},
	};

	currentDistance_ = 0.0f;
	moveSpeed_ = 1.0f;

	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.SetRotate({0.0f, 0.0f, 0.0f});

	BuildLengthTable();
	railPosition_ = GetLoopSplinePosition(
		controlPoints_, GetTFromDistance(currentDistance_));
	targetPosition_ = GetLoopSplinePosition(
		controlPoints_, GetTFromDistance(currentDistance_ + targetRange_));
	UpdateTransforms();
}

void RailRoot::Update(float deltaTime)
{
	if (totalLength_ <= 0.0f)
	{
		return;
	}

#ifdef _DEBUG
	ImGui::Begin("RailRoot");
	ImGui::DragFloat("MoveSpeed", &moveSpeed_, 0.1f);
	ImGui::InputFloat3(
		"RailPosition", &railPosition_.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
	ImGui::InputFloat(
		"Distance", &currentDistance_, 0.0f, 0.0f, "%.3f",
		ImGuiInputTextFlags_ReadOnly);
	ImGui::End();
#endif

	currentDistance_ = std::fmod(
		currentDistance_ + moveSpeed_ * deltaTime, totalLength_);
	if (currentDistance_ < 0.0f)
	{
		currentDistance_ += totalLength_;
	}

	railPosition_ = GetLoopSplinePosition(
		controlPoints_, GetTFromDistance(currentDistance_));
	targetPosition_ = GetLoopSplinePosition(
		controlPoints_, GetTFromDistance(currentDistance_ + targetRange_));
	UpdateTransforms();
}

void RailRoot::Finalize()
{
	worldTransform_.Finalize();
}

void RailRoot::DrawDebugUI()
{
#ifdef _DEBUG

	DebugDrawManager* debugDraw = DebugDrawManager::GetInstance();
	const Matrix4x4& railWorldMatrix = worldTransform_.worldMatrix_;
	const Vector3 origin =
	{
		railWorldMatrix.m[3][0],
		railWorldMatrix.m[3][1],
		railWorldMatrix.m[3][2]
	};
	const Vector3 xAxis = Normalize(
		TransformNormal({1.0f, 0.0f, 0.0f}, railWorldMatrix));
	const Vector3 yAxis = Normalize(
		TransformNormal({0.0f, 1.0f, 0.0f}, railWorldMatrix));
	const Vector3 zAxis = Normalize(
		TransformNormal({0.0f, 0.0f, 1.0f}, railWorldMatrix));
	constexpr float kAxisLength = 2.0f;

	debugDraw->AddSphere(
		origin, 0.25f, {1.0f, 1.0f, 0.0f, 1.0f}, 12);
	debugDraw->AddLine(
		origin, origin + xAxis * kAxisLength,
		{1.0f, 0.0f, 0.0f, 1.0f});
	debugDraw->AddLine(
		origin, origin + yAxis * kAxisLength,
		{0.0f, 1.0f, 0.0f, 1.0f});
	debugDraw->AddLine(
		origin, origin + zAxis * kAxisLength,
		{0.0f, 0.0f, 1.0f, 1.0f});
	debugDraw->AddSphere(
		targetPosition_, 0.2f, {0.0f, 1.0f, 1.0f, 1.0f}, 12);
	debugDraw->AddLoopSpline(
		controlPoints_, {1.0f, 0.0f, 0.0f, 1.0f}, 300);
#endif
}

void RailRoot::BuildLengthTable()
{
	lengthTable_.clear();
	totalLength_ = 0.0f;

	constexpr uint32_t kSampleCount = 400;
	Vector3 previousPosition = GetLoopSplinePosition(controlPoints_, 0.0f);
	lengthTable_.push_back({0.0f, 0.0f});

	for (uint32_t i = 1; i <= kSampleCount; ++i)
	{
		const float t =
			static_cast<float>(i) / static_cast<float>(kSampleCount);
		const Vector3 currentPosition =
			GetLoopSplinePosition(controlPoints_, t);

		totalLength_ += Length(currentPosition - previousPosition);
		lengthTable_.push_back({totalLength_, t});
		previousPosition = currentPosition;
	}
}

float RailRoot::GetTFromDistance(float distance) const
{
	if (lengthTable_.empty() || totalLength_ <= 0.0f)
	{
		return 0.0f;
	}

	distance = std::fmod(distance, totalLength_);
	if (distance < 0.0f)
	{
		distance += totalLength_;
	}

	const auto it = std::lower_bound(
		lengthTable_.begin(), lengthTable_.end(), distance,
		[](const SplineSample& sample, float value)
		{
			return sample.distance < value;
		});

	if (it == lengthTable_.begin())
	{
		return lengthTable_.front().t;
	}
	if (it == lengthTable_.end())
	{
		return lengthTable_.back().t;
	}

	const SplineSample& previousSample = *(it - 1);
	const SplineSample& nextSample = *it;
	const float segmentLength =
		nextSample.distance - previousSample.distance;
	if (segmentLength < 0.0001f)
	{
		return previousSample.t;
	}

	const float factor =
		(distance - previousSample.distance) / segmentLength;
	return previousSample.t +
		(nextSample.t - previousSample.t) * factor;
}

void RailRoot::UpdateTransforms()
{
	Vector3 forward = targetPosition_ - railPosition_;
	if (Length(forward) <= 0.00001f)
	{
		forward = {0.0f, 0.0f, 1.0f};
	}
	else
	{
		forward = Normalize(forward);
	}

	Vector3 rotate{};
	rotate.y = std::atan2(forward.x, forward.z);
	rotate.x = std::atan2(
		-forward.y, Length(Vector2{forward.x, forward.z}));

	worldTransform_.translate_ = railPosition_;
	worldTransform_.SetRotate(rotate);
	worldTransform_.UpdateMatrix();
}
