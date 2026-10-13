#define NOMINMAX
#include "EnemyStageConfig.h"

#include "Logger.h"
#include "ResourcePath.h"
#include "json.hpp"

#include <algorithm>
#include <fstream>

namespace
{
	using json = nlohmann::json;

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
			value[2].get<float>()
		};
	}

	Vector4 ReadVector4(const json& value, const Vector4& fallback)
	{
		if (!value.is_array() || value.size() != 4)
		{
			return fallback;
		}
		return
		{
			value[0].get<float>(),
			value[1].get<float>(),
			value[2].get<float>(),
			value[3].get<float>()
		};
	}

	EnemyType ReadEnemyType(const std::string& value)
	{
		if (value == "vertical")
		{
			return EnemyType::Vertical;
		}
		if (value == "horizontal")
		{
			return EnemyType::Horizontal;
		}
		return EnemyType::Stationary;
	}

	void ApplyEnemyValues(const json& source, EnemyCreateConfig& config)
	{
		if (source.contains("scale"))
		{
			config.scale = ReadVector3(source["scale"], config.scale);
		}
		if (source.contains("position"))
		{
			config.position = ReadVector3(source["position"], config.position);
		}
		if (source.contains("color"))
		{
			config.color = ReadVector4(source["color"], config.color);
		}
		config.hitPoint = source.value("hitPoint", config.hitPoint);
		config.colliderRadius = source.value("colliderRadius", config.colliderRadius);
		config.moveAmplitude = source.value("moveAmplitude", config.moveAmplitude);
		config.moveSpeed = source.value("moveSpeed", config.moveSpeed);
		config.movePhase = source.value("movePhase", config.movePhase);
		config.fadeNearDistance =
			source.value("fadeNearDistance", config.fadeNearDistance);
		config.fadeFarDistance =
			source.value("fadeFarDistance", config.fadeFarDistance);
		config.minimumAlpha = source.value("minimumAlpha", config.minimumAlpha);
	}
}

EnemyStageConfig EnemyStageConfig::CreateDefault()
{
	EnemyStageConfig config;
	config.smallEnemyPhaseDistance = 64.6188f;

	EnemyCreateConfig smallDefault;
	smallDefault.type = EnemyType::Stationary;
	smallDefault.scale = { 1.0f, 1.0f, 1.0f };
	smallDefault.position = { 0.0f, 0.0f, 10.0f };
	smallDefault.color = { 1.0f, 0.35f, 0.35f, 1.0f };
	smallDefault.hitPoint = 30;
	smallDefault.colliderRadius = 1.25f;
	smallDefault.moveAmplitude = 3.0f;
	smallDefault.moveSpeed = 2.0f;
	smallDefault.fadeNearDistance = 2.0f;
	smallDefault.fadeFarDistance = 7.0f;
	smallDefault.minimumAlpha = 0.15f;

	const auto add = [&config, &smallDefault](
		float railRatio, EnemyType type, const Vector3& position, float movePhase)
		{
			SmallEnemySpawnConfig spawn;
			spawn.railRatio = railRatio;
			spawn.enemy = smallDefault;
			spawn.enemy.type = type;
			spawn.enemy.position = position;
			spawn.enemy.movePhase = movePhase;
			if (type == EnemyType::Vertical)
			{
				spawn.enemy.color = { 0.35f, 1.0f, 0.45f, 1.0f };
			}
			else if (type == EnemyType::Horizontal)
			{
				spawn.enemy.color = { 0.35f, 0.55f, 1.0f, 1.0f };
			}
			config.smallEnemies.push_back(spawn);
		};

	add(0.00f, EnemyType::Stationary, { -5.0f, 0.0f, 10.0f }, 0.0f);
	add(0.10f, EnemyType::Vertical, { 0.0f, 0.0f, 12.0f }, 0.0f);
	add(0.20f, EnemyType::Horizontal, { 0.0f, 2.0f, 12.0f }, 0.0f);
	add(0.30f, EnemyType::Stationary, { 5.0f, 1.0f, 10.0f }, 0.0f);
	add(0.40f, EnemyType::Vertical, { -3.0f, 0.0f, 13.0f }, 1.57f);
	add(0.50f, EnemyType::Horizontal, { 0.0f, -1.0f, 13.0f }, 1.57f);
	add(0.55f, EnemyType::Stationary, { -5.0f, 0.0f, 10.0f }, 0.0f);
	add(0.63f, EnemyType::Vertical, { 0.0f, 0.0f, 12.0f }, 0.0f);
	add(0.71f, EnemyType::Horizontal, { 0.0f, 2.0f, 12.0f }, 0.0f);
	add(0.79f, EnemyType::Stationary, { 5.0f, 1.0f, 10.0f }, 0.0f);
	add(0.87f, EnemyType::Vertical, { -3.0f, 0.0f, 13.0f }, 1.57f);
	add(0.95f, EnemyType::Horizontal, { 0.0f, -1.0f, 13.0f }, 1.57f);

	config.boss = EnemyCreateConfig{};
	return config;
}

bool EnemyStageConfig::Load(const std::string& relativePath)
{
	std::ifstream input(ResourcePath::MakeString(relativePath));
	if (!input.is_open())
	{
		Logger::Log("Enemy stage JSON was not found. Using built-in defaults.\n");
		return false;
	}

	try
	{
		json root;
		input >> root;

		EnemyStageConfig loaded = CreateDefault();
		loaded.smallEnemyPhaseDistance =
			std::max(root.value("smallEnemyPhaseDistance", loaded.smallEnemyPhaseDistance), 0.0f);

		EnemyCreateConfig smallDefault = loaded.smallEnemies.front().enemy;
		if (root.contains("smallEnemyDefaults"))
		{
			ApplyEnemyValues(root["smallEnemyDefaults"], smallDefault);
		}

		if (root.contains("smallEnemies") && root["smallEnemies"].is_array())
		{
			loaded.smallEnemies.clear();
			for (const json& value : root["smallEnemies"])
			{
				SmallEnemySpawnConfig spawn;
				spawn.railRatio = std::clamp(value.value("railRatio", 0.0f), 0.0f, 1.0f);
				spawn.enemy = smallDefault;
				spawn.enemy.type = ReadEnemyType(value.value("type", "stationary"));
				ApplyEnemyValues(value, spawn.enemy);
				loaded.smallEnemies.push_back(spawn);
			}
		}

		if (root.contains("boss"))
		{
			ApplyEnemyValues(root["boss"], loaded.boss);
		}
		loaded.boss.type = EnemyType::Boss;

		std::sort(loaded.smallEnemies.begin(), loaded.smallEnemies.end(),
			[](const SmallEnemySpawnConfig& lhs, const SmallEnemySpawnConfig& rhs)
			{
				return lhs.railRatio < rhs.railRatio;
			});

		*this = std::move(loaded);
		return true;
	}
	catch (const std::exception& exception)
	{
		Logger::Log(std::string("Failed to load enemy stage JSON: ") +
			exception.what() + "\nUsing built-in defaults.\n");
		return false;
	}
}
