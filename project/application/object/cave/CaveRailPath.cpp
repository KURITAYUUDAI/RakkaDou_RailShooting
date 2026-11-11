#define NOMINMAX
#include "CaveRailPath.h"

#include "Logger.h"
#include "ResourcePath.h"
#include "SplineCurve.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace
{
	constexpr float kEpsilon = 0.00001f;

	Vector3 ReadVector3(const json& value, const Vector3& fallback)
	{
		if (!value.is_array() || value.size() != 3)
		{
			return fallback;
		}
		return
		{
			value[0].get<float>(),
			value[1].get<float>(),
			value[2].get<float>(),
		};
	}

	std::vector<Vector3> ReadControlPoints(
		const json& value, const std::vector<Vector3>& fallback, size_t minimumCount)
	{
		if (!value.is_array())
		{
			return fallback;
		}

		std::vector<Vector3> points;
		points.reserve(value.size());
		for (const json& point : value)
		{
			if (!point.is_array() || point.size() != 3)
			{
				return fallback;
			}
			points.push_back(ReadVector3(point, {}));
		}
		return points.size() >= minimumCount ? points : fallback;
	}

	Vector3 RotateAroundAxis(const Vector3& value, const Vector3& axis, float angle)
	{
		const float cosine = std::cos(angle);
		const float sine = std::sin(angle);
		return value * cosine + Cross(axis, value) * sine +
			axis * Dot(axis, value) * (1.0f - cosine);
	}

	Vector3 TransportNormal(
		const Vector3& normal, const Vector3& previousTangent, const Vector3& tangent)
	{
		const Vector3 axis = Cross(previousTangent, tangent);
		const float axisLength = Length(axis);
		Vector3 transported = normal;
		if (axisLength > kEpsilon)
		{
			const Vector3 normalizedAxis = axis * (1.0f / axisLength);
			const float angle = std::atan2(
				axisLength, std::clamp(Dot(previousTangent, tangent), -1.0f, 1.0f));
			transported = RotateAroundAxis(normal, normalizedAxis, angle);
		}

		transported -= tangent * Dot(transported, tangent);
		const float length = Length(transported);
		if (length <= kEpsilon)
		{
			const Vector3 fallback = std::abs(tangent.x) < 0.9f ?
				Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 0.0f, 1.0f };
			transported = Normalize(fallback - tangent * Dot(fallback, tangent));
		}
		else
		{
			transported = transported * (1.0f / length);
		}
		return transported;
	}
}

CaveStageSettings CaveStageSettings::CreateDefault()
{
	CaveStageSettings settings;
	settings.entryControlPoints =
	{
		{ 0.0f, 500.0f, 0.0f },
		{ 0.0f, 300.0f, 0.0f },
		{ 0.0f, 100.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
	};
	settings.repeatControlPoints =
	{
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, -150.0f, 0.0f },
		{ 25.0f, -300.0f, 35.0f },
		{ 55.0f, -450.0f, 10.0f },
		{ 20.0f, -600.0f, -40.0f },
		{ -30.0f, -750.0f, -60.0f },
		{ -65.0f, -900.0f, -20.0f },
		{ -40.0f, -1050.0f, 35.0f },
		{ 10.0f, -1200.0f, 70.0f },
		{ 60.0f, -1350.0f, 35.0f },
		{ 70.0f, -1500.0f, -20.0f },
		{ 30.0f, -1650.0f, -65.0f },
		{ -25.0f, -1800.0f, -50.0f },
		{ -65.0f, -1950.0f, 0.0f },
		{ -35.0f, -2100.0f, 45.0f },
		{ 0.0f, -2250.0f, 0.0f },
		{ 0.0f, -2400.0f, 0.0f },
	};
	return settings;
}

bool CaveStageSettings::Load(const std::string& relativePath)
{
	*this = CreateDefault();
	std::ifstream input(ResourcePath::MakeString(relativePath));
	if (!input.is_open())
	{
		Logger::Log("Cave stage JSON was not found. Using built-in defaults.\n");
		return false;
	}

	try
	{
		json root;
		input >> root;

		if (root.contains("rail") && root["rail"].is_object())
		{
			const json& rail = root["rail"];
			entryControlPoints = ReadControlPoints(
				rail.value("entryControlPoints", json::array()), entryControlPoints, 2);
			repeatControlPoints = ReadControlPoints(
				rail.value("repeatControlPoints", json::array()), repeatControlPoints, 4);
			if (rail.contains("repeatOffset"))
			{
				repeatOffset = ReadVector3(rail["repeatOffset"], repeatOffset);
			}
			samplesPerSection = std::clamp(
				rail.value("samplesPerSection", samplesPerSection), 8u, 512u);
		}

		if (root.contains("field") && root["field"].is_object())
		{
			const json& value = root["field"];
			field.spacing = std::clamp(value.value("spacing", field.spacing), 1.0f, 10.0f);
			field.poolCount = std::clamp<size_t>(
				value.value("poolCount", field.poolCount), 8, 256);
			field.behindDistance = std::max(
				value.value("behindDistance", field.behindDistance), 0.0f);
		}

		if (root.contains("crystal") && root["crystal"].is_object())
		{
			const json& value = root["crystal"];
			crystal.enabled = value.value("enabled", crystal.enabled);
			const std::string spacingMode = value.value("spacingMode", "random");
			crystal.spacingMode = spacingMode == "fixed" ?
				CrystalSpacingMode::Fixed : CrystalSpacingMode::Random;
			crystal.fixedInterval = std::max(
				value.value("fixedInterval", crystal.fixedInterval), 1.0f);
			crystal.randomMinInterval = std::max(
				value.value("randomMinInterval", crystal.randomMinInterval), 1.0f);
			crystal.randomMaxInterval = std::max(
				value.value("randomMaxInterval", crystal.randomMaxInterval), 1.0f);
			if (crystal.randomMinInterval > crystal.randomMaxInterval)
			{
				std::swap(crystal.randomMinInterval, crystal.randomMaxInterval);
			}
			crystal.wallRadius = std::max(
				value.value("wallRadius", crystal.wallRadius), 0.0f);
			crystal.scaleMin = std::max(value.value("scaleMin", crystal.scaleMin), 0.01f);
			crystal.scaleMax = std::max(value.value("scaleMax", crystal.scaleMax), 0.01f);
			if (crystal.scaleMin > crystal.scaleMax)
			{
				std::swap(crystal.scaleMin, crystal.scaleMax);
			}
			crystal.entranceClearance = std::max(
				value.value("entranceClearance", crystal.entranceClearance), 0.0f);
			crystal.seed = value.value("seed", crystal.seed);
		}
		return true;
	}
	catch (const std::exception& exception)
	{
		Logger::Log(std::string("Failed to load cave stage JSON: ") +
			exception.what() + "\nUsing built-in defaults.\n");
		*this = CreateDefault();
		return false;
	}
}

bool CaveRailPath::Initialize(const std::string& configPath)
{
	CaveStageSettings settings;
	const bool loaded = settings.Load(configPath);
	Initialize(settings);
	return loaded;
}

void CaveRailPath::Initialize(const CaveStageSettings& settings)
{
	settings_ = settings;
	entryTable_ = BuildTable(
		settings_.entryControlPoints, { 1.0f, 0.0f, 0.0f }, false);
	const Vector3 repeatInitialNormal = entryTable_.samples.empty() ?
		Vector3{ 1.0f, 0.0f, 0.0f } : entryTable_.samples.back().normal;
	repeatTable_ = BuildTable(
		settings_.repeatControlPoints, repeatInitialNormal, true);
}

Vector3 CaveRailPath::GetPosition(float distance) const
{
	return GetFrame(distance).position;
}

RailFrame CaveRailPath::GetFrame(float distance) const
{
	distance = std::max(distance, 0.0f);
	if (distance <= entryTable_.length || repeatTable_.length <= kEpsilon)
	{
		return SampleTable(entryTable_, distance);
	}

	const float repeatDistance = distance - entryTable_.length;
	const float cycleValue = std::floor(repeatDistance / repeatTable_.length);
	const int64_t cycle = static_cast<int64_t>(cycleValue);
	float localDistance = repeatDistance - cycleValue * repeatTable_.length;
	if (localDistance < 0.0f)
	{
		localDistance += repeatTable_.length;
	}

	RailFrame frame = SampleTable(repeatTable_, localDistance);
	frame.position += settings_.repeatOffset * static_cast<float>(cycle);
	return frame;
}

CaveRailPath::PathTable CaveRailPath::BuildTable(
	const std::vector<Vector3>& controlPoints,
	const Vector3& initialNormal,
	bool closeFrameSeam) const
{
	PathTable table;
	if (controlPoints.size() < 2)
	{
		return table;
	}

	const size_t sampleCount =
		(controlPoints.size() - 1) * settings_.samplesPerSection;
	table.samples.resize(sampleCount + 1);
	for (size_t i = 0; i <= sampleCount; ++i)
	{
		const float t = static_cast<float>(i) / static_cast<float>(sampleCount);
		table.samples[i].position = GetSplinePosition(controlPoints, t);
		if (i > 0)
		{
			table.length += Length(
				table.samples[i].position - table.samples[i - 1].position);
		}
		table.samples[i].distance = table.length;
	}

	for (size_t i = 0; i < table.samples.size(); ++i)
	{
		const size_t previous = i > 0 ? i - 1 : i;
		const size_t next = i + 1 < table.samples.size() ? i + 1 : i;
		Vector3 tangent = table.samples[next].position - table.samples[previous].position;
		table.samples[i].tangent = Length(tangent) > kEpsilon ?
			Normalize(tangent) : Vector3{ 0.0f, -1.0f, 0.0f };
	}

	Vector3 normal = initialNormal -
		table.samples.front().tangent * Dot(initialNormal, table.samples.front().tangent);
	if (Length(normal) <= kEpsilon)
	{
		normal = { 1.0f, 0.0f, 0.0f };
	}
	normal = Normalize(normal);
	table.samples.front().normal = normal;
	for (size_t i = 1; i < table.samples.size(); ++i)
	{
		normal = TransportNormal(
			normal, table.samples[i - 1].tangent, table.samples[i].tangent);
		table.samples[i].normal = normal;
	}

	if (closeFrameSeam && table.length > kEpsilon)
	{
		const Vector3& endTangent = table.samples.back().tangent;
		const Vector3& endNormal = table.samples.back().normal;
		const Vector3& startNormal = table.samples.front().normal;
		const float correction = std::atan2(
			Dot(Cross(endNormal, startNormal), endTangent),
			std::clamp(Dot(endNormal, startNormal), -1.0f, 1.0f));
		for (PathSample& sample : table.samples)
		{
			const float progress = sample.distance / table.length;
			sample.normal = Normalize(RotateAroundAxis(
				sample.normal, sample.tangent, correction * progress));
		}
	}

	for (PathSample& sample : table.samples)
	{
		sample.binormal = Normalize(Cross(sample.tangent, sample.normal));
		sample.normal = Normalize(Cross(sample.binormal, sample.tangent));
		sample.rotation = MakeRailRotation(
			sample.tangent, sample.normal, sample.binormal);
	}
	return table;
}

RailFrame CaveRailPath::SampleTable(const PathTable& table, float distance) const
{
	RailFrame frame;
	if (table.samples.empty())
	{
		return frame;
	}

	distance = std::clamp(distance, 0.0f, table.length);
	auto next = std::lower_bound(
		table.samples.begin(), table.samples.end(), distance,
		[](const PathSample& sample, float value)
		{
			return sample.distance < value;
		});
	if (next == table.samples.begin())
	{
		const PathSample& sample = table.samples.front();
		return { sample.position, sample.tangent, sample.normal,
			sample.binormal, sample.rotation };
	}
	if (next == table.samples.end())
	{
		const PathSample& sample = table.samples.back();
		return { sample.position, sample.tangent, sample.normal,
			sample.binormal, sample.rotation };
	}

	const PathSample& previous = *(next - 1);
	const float range = next->distance - previous.distance;
	const float t = range > kEpsilon ?
		(distance - previous.distance) / range : 0.0f;
	frame.position = previous.position * (1.0f - t) + next->position * t;
	frame.rotation = Slerp(previous.rotation, next->rotation, t);
	frame.normal = Normalize(RotateVector({ 1.0f, 0.0f, 0.0f }, frame.rotation));
	frame.tangent = Normalize(RotateVector({ 0.0f, -1.0f, 0.0f }, frame.rotation));
	frame.binormal = Normalize(RotateVector({ 0.0f, 0.0f, 1.0f }, frame.rotation));
	return frame;
}

Quaternion CaveRailPath::MakeRailRotation(
	const Vector3& tangent, const Vector3& normal, const Vector3& binormal)
{
	Matrix4x4 matrix = MakeIdentity4x4();
	matrix.m[0][0] = normal.x;
	matrix.m[0][1] = normal.y;
	matrix.m[0][2] = normal.z;
	matrix.m[1][0] = -tangent.x;
	matrix.m[1][1] = -tangent.y;
	matrix.m[1][2] = -tangent.z;
	matrix.m[2][0] = binormal.x;
	matrix.m[2][1] = binormal.y;
	matrix.m[2][2] = binormal.z;
	// MakeFromMatrixは列ベクトル基底を前提とするため、
	// 行ベクトル基底で組み立てた行列を転置して渡す。
	return Normalize(MakeFromMatrix(Transpose(matrix)));
}
