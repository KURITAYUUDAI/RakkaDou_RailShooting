#pragma once

#include "Quaternion.h"
#include "myMath.h"

#include <cstdint>
#include <string>
#include <vector>

struct RailFrame
{
	Vector3 position{};
	Vector3 tangent{ 0.0f, -1.0f, 0.0f };
	Vector3 normal{ 1.0f, 0.0f, 0.0f };
	Vector3 binormal{ 0.0f, 0.0f, 1.0f };
	Quaternion rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
};

enum class CrystalSpacingMode
{
	Fixed,
	Random,
};

struct CaveFieldSettings
{
	float spacing = 9.9f;
	size_t poolCount = 72;
	float behindDistance = 80.0f;
};

struct CaveCrystalSettings
{
	bool enabled = true;
	CrystalSpacingMode spacingMode = CrystalSpacingMode::Random;
	float fixedInterval = 30.0f;
	float randomMinInterval = 18.0f;
	float randomMaxInterval = 45.0f;
	float wallRadius = 29.5f;
	float scaleMin = 0.5f;
	float scaleMax = 1.1f;
	float entranceClearance = 30.0f;
	uint32_t seed = 1129466195u;
};

struct CaveStageSettings
{
	std::vector<Vector3> entryControlPoints;
	std::vector<Vector3> repeatControlPoints;
	Vector3 repeatOffset{ 0.0f, -2400.0f, 0.0f };
	uint32_t samplesPerSection = 64;
	CaveFieldSettings field;
	CaveCrystalSettings crystal;

	static CaveStageSettings CreateDefault();
	bool Load(const std::string& relativePath);
};

class CaveRailPath
{
public:
	bool Initialize(const std::string& configPath = "Field/cave_stage.json");
	void Initialize(const CaveStageSettings& settings);

	Vector3 GetPosition(float distance) const;
	RailFrame GetFrame(float distance) const;
	float GetTotalEntryLength() const { return entryTable_.length; }
	float GetRepeatLength() const { return repeatTable_.length; }
	const CaveStageSettings& GetSettings() const { return settings_; }

private:
	struct PathSample
	{
		float distance = 0.0f;
		Vector3 position{};
		Vector3 tangent{ 0.0f, -1.0f, 0.0f };
		Vector3 normal{ 1.0f, 0.0f, 0.0f };
		Vector3 binormal{ 0.0f, 0.0f, 1.0f };
		Quaternion rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct PathTable
	{
		std::vector<PathSample> samples;
		float length = 0.0f;
	};

	PathTable BuildTable(
		const std::vector<Vector3>& controlPoints,
		const Vector3& initialNormal,
		bool closeFrameSeam) const;
	RailFrame SampleTable(const PathTable& table, float distance) const;
	static Quaternion MakeRailRotation(
		const Vector3& tangent, const Vector3& normal, const Vector3& binormal);

	CaveStageSettings settings_ = CaveStageSettings::CreateDefault();
	PathTable entryTable_;
	PathTable repeatTable_;
};
