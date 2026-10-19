#define NOMINMAX
#include "EnemyStageConfig.h"

#include "Logger.h"
#include "ResourcePath.h"
#include "json.hpp"

#include <algorithm>
#include <array>
#include <fstream>

namespace
{
	using json = nlohmann::json;
	constexpr size_t kSmallEnemyRoleCount = 5;
	using SmallEnemyRoleConfigs = std::array<EnemyCreateConfig, kSmallEnemyRoleCount>;

	size_t GetRoleIndex(SmallEnemyRole role)
	{
		return static_cast<size_t>(role);
	}

	Vector3 ReadVector3(const json& value, const Vector3& fallback)
	{
		if (!value.is_array() || value.size() != 3)
		{
			return fallback;
		}
		return { value[0].get<float>(), value[1].get<float>(), value[2].get<float>() };
	}

	Vector4 ReadVector4(const json& value, const Vector4& fallback)
	{
		if (!value.is_array() || value.size() != 4)
		{
			return fallback;
		}
		return
		{
			value[0].get<float>(), value[1].get<float>(),
			value[2].get<float>(), value[3].get<float>()
		};
	}

	Vector3 ReadSmallEnemyPosition(const json& value, const Vector3& fallback)
	{
		if (!value.is_array())
		{
			return fallback;
		}
		if (value.size() == 2)
		{
			return { value[0].get<float>(), value[1].get<float>(), fallback.z };
		}
		return ReadVector3(value, fallback);
	}

	SmallEnemyRole ReadSmallEnemyRole(const std::string& value, bool& recognized)
	{
		recognized = true;
		if (value == "basic") { return SmallEnemyRole::Basic; }
		if (value == "turret") { return SmallEnemyRole::Turret; }
		if (value == "charger") { return SmallEnemyRole::Charger; }
		if (value == "approacher") { return SmallEnemyRole::Approacher; }
		if (value == "elite") { return SmallEnemyRole::Elite; }

		recognized = false;
		return SmallEnemyRole::Basic;
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

		config.hitPoint = std::max(source.value("hitPoint", config.hitPoint), 1);
		config.colliderRadius = std::max(source.value("colliderRadius", config.colliderRadius), 0.0f);
		config.spawnFogMargin = std::max(
			source.value("spawnFogMargin", config.spawnFogMargin), 0.0f);
		config.spawnApproachSpeed = std::max(
			source.value("spawnApproachSpeed", config.spawnApproachSpeed), 0.0f);
		config.spawnSettleDuration = std::max(
			source.value("spawnSettleDuration", config.spawnSettleDuration), 0.0f);
		config.forwardSpeed = std::max(
			source.value("forwardSpeed", config.forwardSpeed), 0.0f);
		config.escapeHorizontalSpeed = std::max(
			source.value("escapeHorizontalSpeed", config.escapeHorizontalSpeed), 0.0f);
		config.despawnHorizontalX = std::max(
			source.value("despawnHorizontalX", config.despawnHorizontalX), 0.0f);
		config.fadeNearDistance = std::max(source.value("fadeNearDistance", config.fadeNearDistance), 0.0f);
		config.fadeFarDistance = std::max(
			source.value("fadeFarDistance", config.fadeFarDistance), config.fadeNearDistance);
		config.minimumAlpha = std::clamp(
			source.value("minimumAlpha", config.minimumAlpha), 0.0f, 1.0f);
		config.shotInterval = std::max(source.value("shotInterval", config.shotInterval), 0.01f);
		config.burstShotInterval = std::max(
			source.value("burstShotInterval", config.burstShotInterval), 0.01f);
		config.bulletSpeed = std::max(source.value("bulletSpeed", config.bulletSpeed), 0.0f);
		config.chargeDuration = std::max(source.value("chargeDuration", config.chargeDuration), 0.0f);
		config.chargeSpeed = std::max(source.value("chargeSpeed", config.chargeSpeed), 0.0f);
		config.rushDuration = std::max(source.value("rushDuration", config.rushDuration), 0.01f);
		config.maxRushDistance = std::max(
			source.value("maxRushDistance", config.maxRushDistance), 0.0f);
		config.approachDelay = std::max(source.value("approachDelay", config.approachDelay), 0.0f);
		config.approachDuration = std::max(
			source.value("approachDuration", config.approachDuration), 0.01f);
		config.approachStepDistance = std::max(
			source.value("approachStepDistance", config.approachStepDistance), 0.01f);
		config.nearDuration = std::max(source.value("nearDuration", config.nearDuration), 0.0f);
		config.escapeSpeed = std::max(source.value("escapeSpeed", config.escapeSpeed), 0.0f);
		config.farLineZ = source.value("farLineZ", config.farLineZ);
		config.nearLineZ = source.value("nearLineZ", config.nearLineZ);
		config.despawnLineZ = source.value("despawnLineZ", config.despawnLineZ);
		config.contactDamage = std::max(source.value("contactDamage", config.contactDamage), 0);
	}

	SmallEnemyRoleConfigs CreateRoleDefaults()
	{
		EnemyCreateConfig common;
		common.type = EnemyType::Small;
		common.scale = { 1.0f, 1.0f, 1.0f };
		common.position = { 0.0f, 0.0f, 12.0f };
		common.colliderRadius = 1.25f;
		common.spawnFogMargin = 20.0f;
		common.spawnApproachSpeed = 100.0f;
		common.spawnSettleDuration = 0.5f;
		common.forwardSpeed = 2.0f;
		common.escapeHorizontalSpeed = 10.0f;
		common.despawnHorizontalX = 12.0f;
		common.fadeNearDistance = 2.0f;
		common.fadeFarDistance = 7.0f;
		common.minimumAlpha = 0.15f;
		common.shotInterval = 3.0f;
		common.burstShotInterval = 0.2f;
		common.bulletSpeed = 0.0f;
		common.chargeDuration = 1.5f;
		common.chargeSpeed = 20.0f;
		common.rushDuration = 0.75f;
		common.maxRushDistance = 30.0f;
		common.approachDelay = 4.0f;
		common.approachDuration = 1.5f;
		common.approachStepDistance = 10.0f;
		common.nearDuration = 4.0f;
		common.escapeSpeed = 12.0f;
		common.farLineZ = 12.0f;
		common.nearLineZ = -2.0f;
		common.despawnLineZ = -15.0f;
		common.contactDamage = 1;

		SmallEnemyRoleConfigs roles;
		for (size_t i = 0; i < roles.size(); ++i)
		{
			roles[i] = common;
			roles[i].role = static_cast<SmallEnemyRole>(i);
		}

		auto& basic = roles[GetRoleIndex(SmallEnemyRole::Basic)];
		basic.hitPoint = 5;
		basic.color = { 0.35f, 0.55f, 1.0f, 1.0f };
		basic.bulletSpeed = 15.0f;

		auto& turret = roles[GetRoleIndex(SmallEnemyRole::Turret)];
		turret.hitPoint = 8;
		turret.color = { 1.0f, 0.65f, 0.2f, 1.0f };
		turret.bulletSpeed = 15.0f;

		auto& charger = roles[GetRoleIndex(SmallEnemyRole::Charger)];
		charger.hitPoint = 10;
		charger.color = { 1.0f, 0.2f, 0.2f, 1.0f };
		charger.bulletSpeed = 0.0f;
		charger.contactDamage = 5;

		auto& approacher = roles[GetRoleIndex(SmallEnemyRole::Approacher)];
		approacher.hitPoint = 15;
		approacher.color = { 0.35f, 1.0f, 0.45f, 1.0f };
		approacher.bulletSpeed = 15.0f;
		approacher.farLineZ = 55.0f;
		approacher.approachStepDistance = 10.0f;

		auto& elite = roles[GetRoleIndex(SmallEnemyRole::Elite)];
		elite.hitPoint = 30;
		elite.color = { 0.75f, 0.25f, 1.0f, 1.0f };
		elite.bulletSpeed = 15.0f;
		elite.nearDuration = 6.0f;
		elite.contactDamage = 5;

		return roles;
	}
}

EnemyStageConfig EnemyStageConfig::CreateDefault()
{
	EnemyStageConfig config;
	config.smallEnemyPhaseDistance = 64.6188f;
	const SmallEnemyRoleConfigs roles = CreateRoleDefaults();

	const auto add = [&config, &roles](
		float railRatio, SmallEnemyRole role, const Vector2& position)
		{
			SmallEnemySpawnConfig spawn;
			spawn.railRatio = railRatio;
			spawn.enemy = roles[GetRoleIndex(role)];
			spawn.enemy.position =
			{
				position.x,
				position.y,
				role == SmallEnemyRole::Charger ?
					spawn.enemy.nearLineZ : spawn.enemy.farLineZ
			};
			config.smallEnemies.push_back(spawn);
		};

	add(0.00f, SmallEnemyRole::Basic, { -4.0f, 0.0f });
	add(0.08f, SmallEnemyRole::Basic, { 4.0f, 1.0f });
	add(0.16f, SmallEnemyRole::Turret, { -5.0f, 0.0f });
	add(0.24f, SmallEnemyRole::Charger, { -3.0f, -3.0f });
	add(0.32f, SmallEnemyRole::Basic, { 0.0f, 2.0f });
	add(0.40f, SmallEnemyRole::Approacher, { -3.0f, -2.0f });
	add(0.48f, SmallEnemyRole::Basic, { 4.0f, -1.0f });
	add(0.56f, SmallEnemyRole::Turret, { 5.0f, 1.0f });
	add(0.64f, SmallEnemyRole::Elite, { 0.0f, 0.0f });
	add(0.74f, SmallEnemyRole::Charger, { 3.0f, -3.0f });
	add(0.84f, SmallEnemyRole::Approacher, { 3.0f, -2.0f });
	add(0.92f, SmallEnemyRole::Basic, { 0.0f, -1.0f });

	config.boss = EnemyCreateConfig{};
	config.boss.type = EnemyType::Boss;
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
		loaded.smallEnemyPhaseDistance = std::max(
			root.value("smallEnemyPhaseDistance", loaded.smallEnemyPhaseDistance), 0.0f);

		SmallEnemyRoleConfigs roleDefaults = CreateRoleDefaults();
		if (root.contains("smallEnemyDefaults"))
		{
			for (EnemyCreateConfig& roleConfig : roleDefaults)
			{
				ApplyEnemyValues(root["smallEnemyDefaults"], roleConfig);
			}
		}

		if (root.contains("smallEnemyRoleDefaults") &&
			root["smallEnemyRoleDefaults"].is_object())
		{
			for (auto iterator = root["smallEnemyRoleDefaults"].begin();
				iterator != root["smallEnemyRoleDefaults"].end(); ++iterator)
			{
				bool recognized = false;
				const SmallEnemyRole role = ReadSmallEnemyRole(iterator.key(), recognized);
				if (!recognized)
				{
					Logger::Log("Unknown small enemy role default: " +
						iterator.key() + ". Ignored.\n");
					continue;
				}
				ApplyEnemyValues(iterator.value(), roleDefaults[GetRoleIndex(role)]);
			}
		}

		if (root.contains("smallEnemies") && root["smallEnemies"].is_array())
		{
			loaded.smallEnemies.clear();
			for (const json& value : root["smallEnemies"])
			{
				const std::string roleName = value.value("role", "basic");
				bool recognized = false;
				const SmallEnemyRole role = ReadSmallEnemyRole(roleName, recognized);
				if (!recognized)
				{
					Logger::Log("Unknown small enemy role: " + roleName +
						". Falling back to basic.\n");
				}

				SmallEnemySpawnConfig spawn;
				spawn.railRatio = std::clamp(
					value.value("railRatio", 0.0f), 0.0f, 1.0f);
				spawn.enemy = roleDefaults[GetRoleIndex(role)];
				spawn.enemy.type = EnemyType::Small;
				spawn.enemy.role = role;
				ApplyEnemyValues(value, spawn.enemy);
				if (value.contains("position"))
				{
					spawn.enemy.position = ReadSmallEnemyPosition(
						value["position"], spawn.enemy.position);
				}
				spawn.enemy.position.z = role == SmallEnemyRole::Charger ?
					spawn.enemy.nearLineZ : spawn.enemy.farLineZ;
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
