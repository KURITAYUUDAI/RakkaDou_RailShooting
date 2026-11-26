#define NOMINMAX
#include "EnemyStageDirector.h"

#include "EventBus.h"
#include "RailCameraController.h"
#include "SkyBox.h"
#include "Terrain.h"

#include <algorithm>
#include <cfloat>
#include <utility>

#ifdef _DEBUG
#include "imgui.h"
#endif

EnemyStageDirector::EnemyStageDirector(
	EnemyStageDependencies dependencies,
	EnemyStageDirectorConfig config)
	: dependencies_(dependencies),
	  directorConfig_(std::move(config))
{
}

void EnemyStageDirector::Initialize()
{
	stageConfig_ = EnemyStageConfig::CreateDefault();
	stageConfig_.Load(directorConfig_.stageConfigPath);
	phase_ = Phase::SmallEnemies;
	nextSmallEnemyIndex_ = 0;
	bossEnemyID_ = 0;

	smallEnemyPoolHandles_.clear();
	smallEnemyPoolHandles_.reserve(stageConfig_.smallEnemies.size());
	for (const SmallEnemySpawnConfig& spawn : stageConfig_.smallEnemies)
	{
		smallEnemyPoolHandles_.push_back(
			dependencies_.enemyManager.PrewarmEnemy(spawn.enemy));
	}
	bossEnemyPoolHandle_ =
		dependencies_.enemyManager.PrewarmEnemy(stageConfig_.boss);

	// railRatio == 0 の敵も開始遅延後に出現させる。
	Update(0.0f);
}

void EnemyStageDirector::Update(float deltaTime)
{
	if (phase_ != Phase::SmallEnemies)
	{
		return;
	}

	static_cast<void>(deltaTime);
	const float phaseDistance = stageConfig_.smallEnemyPhaseDistance;
	const float spawnDelay = std::max(0.0f, directorConfig_.spawnDelay);
	const float travelledDistance =
		dependencies_.railCamera.GetTravelledDistance();

	while (nextSmallEnemyIndex_ < stageConfig_.smallEnemies.size())
	{
		const SmallEnemySpawnConfig& spawn =
			stageConfig_.smallEnemies[nextSmallEnemyIndex_];
		const float spawnDistance =
			spawnDelay + spawn.railRatio * phaseDistance;
		if (travelledDistance < spawnDistance)
		{
			break;
		}

		SpawnSmallEnemy(spawn, nextSmallEnemyIndex_);
		++nextSmallEnemyIndex_;
	}

	if (travelledDistance >= GetBossPhaseDistance())
	{
		StartBossPhase();
	}
}

void EnemyStageDirector::SkipToBossPhase()
{
	if (phase_ != Phase::SmallEnemies)
	{
		return;
	}

	dependencies_.railCamera.SetTravelledDistance(GetBossPhaseDistance());
}

bool EnemyStageDirector::IsCurrentBoss(EnemyID enemyID) const
{
	return phase_ == Phase::Boss && bossEnemyID_ != 0 &&
		enemyID == bossEnemyID_;
}

bool EnemyStageDirector::IsBossPhase() const
{
	return phase_ == Phase::Boss;
}

void EnemyStageDirector::SpawnSmallEnemy(
	const SmallEnemySpawnConfig& spawnConfig,
	size_t spawnIndex)
{
	EnemyID id = 0;
	if (spawnIndex < smallEnemyPoolHandles_.size())
	{
		id = dependencies_.enemyManager.ActivatePrewarmedEnemy(
			smallEnemyPoolHandles_[spawnIndex]);
	}
	if (id == 0)
	{
		id = dependencies_.enemyManager.AddEnemy(spawnConfig.enemy);
	}
	ConfigureEnemy(id, false);
}

void EnemyStageDirector::StartBossPhase()
{
	dependencies_.railCamera.SetMovementEnabled(false);
	dependencies_.terrain.SetGameplayCaveScrolling(true);

	dependencies_.enemyManager.RemoveSmallEnemies();
	bossEnemyID_ = dependencies_.enemyManager.ActivatePrewarmedEnemy(
		bossEnemyPoolHandle_);
	if (bossEnemyID_ == 0)
	{
		bossEnemyID_ = dependencies_.enemyManager.AddEnemy(stageConfig_.boss);
	}
	ConfigureEnemy(bossEnemyID_, true);
	phase_ = Phase::Boss;
}

void EnemyStageDirector::ConfigureEnemy(EnemyID id, bool isBoss)
{
	Enemy* enemy = dependencies_.enemyManager.FindEnemy(id);
	if (!enemy)
	{
		return;
	}

	enemy->SetEnvironmentTextureIndex(
		dependencies_.skyBox.GetEnvironmentTextureIndex());
	if (!isBoss)
	{
		enemy->SetContinuousRailFollowEnabled(
			stageConfig_.smallEnemyContinuousRailFollowEnabled);
	}
	enemy->SetParent(
		dependencies_.railCamera.GetBattleWorldTransform());
	enemy->SetEventBus(&dependencies_.eventBus);
	if (isBoss)
	{
		enemy->SetHPGageDisplayType(EnemyHPGageDisplayType::ScreenBoss);
		enemy->SetScreenBossPriority(50);
	}
	else
	{
		enemy->SetHPGageDisplayType(EnemyHPGageDisplayType::OverHead);
	}
	enemy->EventDispatch();
}

float EnemyStageDirector::GetBossPhaseDistance() const
{
	return std::max(0.0f, directorConfig_.spawnDelay) +
		stageConfig_.smallEnemyPhaseDistance;
}

void EnemyStageDirector::DrawDebugUI() const
{
#ifdef _DEBUG
	const float travelledDistance =
		dependencies_.railCamera.GetTravelledDistance();
	const float phaseDistance = stageConfig_.smallEnemyPhaseDistance;
	const float spawnDelay = std::max(0.0f, directorConfig_.spawnDelay);
	const float progress = phaseDistance > 0.0f ?
		std::clamp(
			(travelledDistance - spawnDelay) / phaseDistance,
			0.0f, 1.0f) : 0.0f;

	ImGui::Begin("Enemy Stage Debug");
	ImGui::Text("Rail travelled distance: %.3f", travelledDistance);
	ImGui::Text("Enemy spawn delay: %.3f", spawnDelay);
	ImGui::Text("Small enemy phase distance: %.3f", phaseDistance);
	ImGui::Text("Progress: %.3f / %.1f%%", progress, progress * 100.0f);
	ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f));
	ImGui::Text("Next spawn index: %zu / %zu",
		nextSmallEnemyIndex_, stageConfig_.smallEnemies.size());

	constexpr ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollX;
	if (ImGui::BeginTable(
		"ActiveEnemies", 7, tableFlags, ImVec2(0.0f, 260.0f)))
	{
		ImGui::TableSetupColumn("Enemy ID");
		ImGui::TableSetupColumn("Role");
		ImGui::TableSetupColumn("State");
		ImGui::TableSetupColumn("Spawn Local");
		ImGui::TableSetupColumn("Current Local");
		ImGui::TableSetupColumn("Max HP");
		ImGui::TableSetupColumn("Current HP");
		ImGui::TableHeadersRow();

		for (const EnemyID id : dependencies_.enemyManager.GetEnemyIDs())
		{
			const Enemy* enemy = dependencies_.enemyManager.FindEnemy(id);
			if (!enemy || enemy->GetIsDead())
			{
				continue;
			}

			const Vector3& spawnPosition = enemy->GetSpawnPosition();
			const Vector3& currentPosition = enemy->GetTranslate();
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%u", static_cast<unsigned int>(id));
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(enemy->GetRoleName());
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(enemy->GetBehaviorStateName());
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("(%.2f, %.2f, %.2f)",
				spawnPosition.x, spawnPosition.y, spawnPosition.z);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("(%.2f, %.2f, %.2f)",
				currentPosition.x, currentPosition.y, currentPosition.z);
			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%d", enemy->GetMaxHitPoint());
			ImGui::TableSetColumnIndex(6);
			ImGui::Text("%d", enemy->GetHitPoint());
		}
		ImGui::EndTable();
	}
	ImGui::End();
#endif
}
