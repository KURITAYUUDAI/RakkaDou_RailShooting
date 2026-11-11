#define NOMINMAX
#include "Terrain.h"
#include "DistanceFogManager.h"
#include "ModelManager.h"
#include "Model.h"
#include "ImGuiManager.h"
#include "cave/CaveRailPath.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kTwoPi = pi * 2.0f;

	uint32_t HashValue(uint32_t value)
	{
		value ^= value >> 16;
		value *= 0x7feb352du;
		value ^= value >> 15;
		value *= 0x846ca68bu;
		value ^= value >> 16;
		return value;
	}

	float HashToUnit(uint32_t seed, uint32_t index, uint32_t channel)
	{
		const uint32_t hash = HashValue(seed ^ HashValue(index + channel * 0x9e3779b9u));
		return static_cast<float>(hash & 0x00ffffffu) /
			static_cast<float>(0x01000000u);
	}

	Matrix4x4 MakeRailFrameMatrix(const RailFrame& frame, const Vector3& offset)
	{
		Matrix4x4 matrix = MakeIdentity4x4();
		matrix.m[0][0] = frame.normal.x;
		matrix.m[0][1] = frame.normal.y;
		matrix.m[0][2] = frame.normal.z;
		matrix.m[1][0] = frame.binormal.x;
		matrix.m[1][1] = frame.binormal.y;
		matrix.m[1][2] = frame.binormal.z;
		matrix.m[2][0] = frame.tangent.x;
		matrix.m[2][1] = frame.tangent.y;
		matrix.m[2][2] = frame.tangent.z;
		matrix.m[3][0] = frame.position.x + offset.x;
		matrix.m[3][1] = frame.position.y + offset.y;
		matrix.m[3][2] = frame.position.z + offset.z;
		return matrix;
	}

	Matrix4x4 MakeCrystalMatrix(
		const RailFrame& frame,
		const Vector3& offset,
		float angle,
		float wallRadius,
		float scale,
		float roll)
	{
		const Vector3 radial = Normalize(
			frame.normal * std::cos(angle) + frame.binormal * std::sin(angle));
		const Vector3 inward = -radial;
		const Vector3 circumferential = Normalize(Cross(frame.tangent, inward));
		const Vector3 localX = Normalize(
			frame.tangent * std::cos(roll) + circumferential * std::sin(roll));
		const Vector3 localZ = Normalize(Cross(localX, inward));
		const Vector3 position = frame.position + offset + radial * wallRadius;

		Matrix4x4 matrix = MakeIdentity4x4();
		matrix.m[0][0] = localX.x * scale;
		matrix.m[0][1] = localX.y * scale;
		matrix.m[0][2] = localX.z * scale;
		matrix.m[1][0] = inward.x * scale;
		matrix.m[1][1] = inward.y * scale;
		matrix.m[1][2] = inward.z * scale;
		matrix.m[2][0] = localZ.x * scale;
		matrix.m[2][1] = localZ.y * scale;
		matrix.m[2][2] = localZ.z * scale;
		matrix.m[3][0] = position.x;
		matrix.m[3][1] = position.y;
		matrix.m[3][2] = position.z;
		return matrix;
	}
}

void Terrain::Initialize(Mode mode, const CaveRailPath* caveRailPath)
{
	mode_ = mode;
	caveRailPath_ = caveRailPath;
	isGameplayCaveScrolling_ = false;
	isGameplayCaveEntryMode_ = false;

	if (mode_ == Mode::GameplayCave && caveRailPath_)
	{
		InitializeGameplayCave();
		return;
	}

	if (mode_ == Mode::TitleCave || mode_ == Mode::GameplayCave)
	{
		ModelManager::GetInstance()->LoadModel("Field", "OutField.obj");
		caveObjects_.reserve(kCaveObjectCount_);
		caveTransforms_.reserve(kCaveObjectCount_);
		caveVisibility_.reserve(kCaveObjectCount_);

		for (size_t i = 0; i < kCaveObjectCount_; ++i)
		{
			auto object = std::make_unique<Object3d>();
			object->Initialize();
			object->SetModel("OutField.obj");
			/*object->SetEnableLighting(false);
			object->SetEnvironmentCoefficient(0.0f);*/

			EulerTransform transform{};
			transform.scale = { 1.0f, 1.0f, 1.0f };
			transform.rotate = { 0.0f, 0.0f, 0.0f };
			if (mode_ == Mode::GameplayCave)
			{
				const float centeredIndex =
					static_cast<float>(kCaveObjectCount_ / 2) - 1.0f - static_cast<float>(i);
				transform.translate = { 0.0f, centeredIndex * kCaveSegmentLength_, 0.0f };
			}
			else
			{
				transform.translate =
					{ 0.0f, -kCaveSegmentLength_ * static_cast<float>(i), 0.0f };
			}

			caveObjects_.push_back(std::move(object));
			caveTransforms_.push_back(transform);
			caveVisibility_.push_back(true);
		}
		return;
	}

	ModelManager::GetInstance()->LoadModel("", "terrain.obj");

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize();
	object3d_->SetModel("terrain.obj");

	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, -5.0f, 0.0f };
}

void Terrain::Update(float deltaTime)
{
	Update(deltaTime, { 0.0f, 0.0f, 0.0f });
}

void Terrain::Update(float deltaTime, const Vector3& referencePosition)

{
	Update(deltaTime, referencePosition, 0.0f, {}, MakeIdentity4x4());
}

void Terrain::Update(
	float deltaTime,
	const Vector3& referencePosition,
	float railDistance,
	const Vector3& pathWorldOffset,
	const Matrix4x4& railFrameWorldMatrix)
{
	if (mode_ == Mode::TitleCave)
	{
		for (size_t i = 0; i < caveObjects_.size(); ++i)
		{
			caveTransforms_[i].translate.y += caveSpeed_ * deltaTime;
			while (caveTransforms_[i].translate.y >= kCaveSegmentLength_)
			{
				caveTransforms_[i].translate.y -= kCaveRecycleDistance_;
			}
			caveObjects_[i]->SetTransform(caveTransforms_[i]);
			caveObjects_[i]->Update();
		}
	}
	else if (mode_ == Mode::GameplayCave)
	{
		if (caveRailPath_)
		{
			UpdateGameplayCave(
				deltaTime, referencePosition, railDistance,
				pathWorldOffset, railFrameWorldMatrix);
		}
		else
		{
		auto* distanceFog = DistanceFogManager::GetInstance();
		const bool isFogEnabled = distanceFog->IsEnabled();
		const float fogEndDistance = distanceFog->GetEndDistance();

		for (size_t i = 0; i < caveObjects_.size(); ++i)
		{
			if (isGameplayCaveScrolling_)
			{
				caveTransforms_[i].translate.y += caveSpeed_ * deltaTime;
			}

			if (!isGameplayCaveEntryMode_)
			{
				while (caveTransforms_[i].translate.y - referencePosition.y > kCaveRecycleHalfDistance_)
				{
					caveTransforms_[i].translate.y -= kCaveRecycleDistance_;
				}
				while (caveTransforms_[i].translate.y - referencePosition.y < -kCaveRecycleHalfDistance_)
				{
					caveTransforms_[i].translate.y += kCaveRecycleDistance_;
				}
			}

			const Vector3 difference = caveTransforms_[i].translate - referencePosition;
			caveVisibility_[i] = !isFogEnabled ||
				Length(difference) - kOutFieldBoundingRadius_ <=
				fogEndDistance + kOutFieldVisibilityMargin_;

			caveObjects_[i]->SetTransform(caveTransforms_[i]);
			caveObjects_[i]->Update();
		}
		}
	}
	else
	{
		object3d_->SetTransform(transform_);
		object3d_->Update();
	}

#ifdef _DEBUG
	ImGui::Begin("TerrainSetting");
	bool isDraw = isDraw_;
	if (ImGui::Checkbox("DrawTerrain", &isDraw))
	{
		isDraw_ = isDraw;
	}
	if (mode_ == Mode::TitleCave || mode_ == Mode::GameplayCave)
	{
		ImGui::DragFloat("CaveSpeed", &caveSpeed_, 0.1f);
	}
	if (mode_ == Mode::GameplayCave)
	{
		ImGui::Checkbox("GameplayCaveScrolling", &isGameplayCaveScrolling_);
		if (caveRailPath_)
		{
			ImGui::Text("ViewerDistance: %.2f", gameplayCaveViewerDistance_);
			ImGui::Text("FieldSegments: %zu", caveObjects_.size());
			ImGui::Text("CrystalPool: %zu", crystalObjects_.size());
		}
	}
	if (mode_ == Mode::TitleCave || mode_ == Mode::GameplayCave)
	{
		for (size_t i = 0; i < caveTransforms_.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::DragFloat3("CavePosition", &caveTransforms_[i].translate.x, 0.1f);
			if (mode_ == Mode::GameplayCave)
			{
				ImGui::Text("Visible: %s", caveVisibility_[i] ? "true" : "false");
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
#endif
}

void Terrain::InitializeGameplayCave()
{
	const CaveStageSettings& settings = caveRailPath_->GetSettings();
	ModelManager::GetInstance()->LoadModel("Field", "Field10m.obj");
	ModelManager::GetInstance()->LoadModel("Field", "Crystal.obj");
	BuildContinuousCaveModel();

	// 1周期がFog距離より十分長いため、前・現在・次の3周期だけを再利用する。
	constexpr size_t kCycleObjectCount = 3;
	caveObjects_.reserve(kCycleObjectCount);
	caveVisibility_.reserve(kCycleObjectCount);
	for (size_t i = 0; i < kCycleObjectCount; ++i)
	{
		auto object = std::make_unique<Object3d>();
		object->Initialize();
		object->SetModel("GameplayCaveTube");
		object->SetTexture("Field/Field.png");
		caveObjects_.push_back(std::move(object));
		caveVisibility_.push_back(true);
	}

	if (settings.crystal.enabled)
	{
		const float coverage = settings.field.spacing *
			static_cast<float>(settings.field.poolCount);
		const float minimumInterval = settings.crystal.spacingMode == CrystalSpacingMode::Fixed ?
			settings.crystal.fixedInterval : settings.crystal.randomMinInterval;
		const size_t crystalPoolCount = static_cast<size_t>(
			std::ceil(coverage / std::max(minimumInterval, 1.0f))) + 4;
		crystalObjects_.reserve(crystalPoolCount);
		crystalVisibility_.reserve(crystalPoolCount);
		for (size_t i = 0; i < crystalPoolCount; ++i)
		{
			auto object = std::make_unique<Object3d>();
			object->Initialize();
			object->SetModel("Crystal.obj");
			crystalObjects_.push_back(std::move(object));
			crystalVisibility_.push_back(false);
		}
	}

	gameplayCaveScrollAnchor_ = MakeIdentity4x4();
}

void Terrain::BuildContinuousCaveModel()
{
	auto* modelManager = ModelManager::GetInstance();
	if (modelManager->FindModel("GameplayCaveTube"))
	{
		return;
	}

	Model* sourceModel = modelManager->FindModel("Field10m.obj");
	if (!sourceModel || sourceModel->GetMeshCount() == 0)
	{
		return;
	}

	const MeshGeometry& source = sourceModel->GetMesh(0);
	const float repeatLength = caveRailPath_->GetRepeatLength();
	constexpr float kSourceSegmentLength = 10.0f;
	const size_t segmentCount = std::max<size_t>(
		1, static_cast<size_t>(std::ceil(repeatLength / kSourceSegmentLength)));
	const float segmentSpan = repeatLength / static_cast<float>(segmentCount);
	const float caveStartDistance = caveRailPath_->GetTotalEntryLength();

	MeshGeometry continuous;
	continuous.materialIndex = 0;
	continuous.vertices.reserve(source.vertices.size() * segmentCount);
	continuous.indices.reserve(source.indices.size() * segmentCount);

	for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
	{
		const float centerDistance = caveStartDistance +
			(static_cast<float>(segmentIndex) + 0.5f) * segmentSpan;
		const uint32_t vertexOffset = static_cast<uint32_t>(continuous.vertices.size());
		for (const VertexData& sourceVertex : source.vertices)
		{
			// Field10mのローカル-Yが進行方向。端のY=+/-5を
			// 隣接区画と同じレール距離へ写し、境界リングを完全一致させる。
			const float railDistance = centerDistance -
				sourceVertex.position.y * (segmentSpan / kSourceSegmentLength);
			const RailFrame frame = caveRailPath_->GetFrame(railDistance);

			VertexData vertex = sourceVertex;
			const Vector3 position = frame.position +
				frame.normal * sourceVertex.position.x +
				frame.binormal * sourceVertex.position.z;
			vertex.position = { position.x, position.y, position.z, 1.0f };
			vertex.normal = Normalize(
				frame.normal * sourceVertex.normal.x -
				frame.tangent * sourceVertex.normal.y +
				frame.binormal * sourceVertex.normal.z);
			continuous.vertices.push_back(vertex);
		}

		for (uint32_t index : source.indices)
		{
			continuous.indices.push_back(vertexOffset + index);
		}
	}

	modelManager->InsertMesh("GameplayCaveTube", std::move(continuous));
}

void Terrain::UpdateGameplayCave(
	float deltaTime,
	const Vector3& referencePosition,
	float railDistance,
	const Vector3& pathWorldOffset,
	const Matrix4x4& railFrameWorldMatrix)
{
	const CaveStageSettings& settings = caveRailPath_->GetSettings();
	gameplayCavePathOffsetY_ = pathWorldOffset.y;
	if (isGameplayCaveScrolling_)
	{
		if (!isGameplayCaveScrollAnchorInitialized_)
		{
			gameplayCaveViewerDistance_ = railDistance;
			gameplayCaveScrollAnchor_ = railFrameWorldMatrix;
			isGameplayCaveScrollAnchorInitialized_ = true;
		}
		gameplayCaveViewerDistance_ += caveSpeed_ * deltaTime;
	}
	else
	{
		gameplayCaveViewerDistance_ = railDistance;
		gameplayCaveScrollAnchor_ = railFrameWorldMatrix;
		isGameplayCaveScrollAnchorInitialized_ = false;
	}

	auto* distanceFog = DistanceFogManager::GetInstance();
	const bool isFogEnabled = distanceFog->IsEnabled();
	const float fogEndDistance = distanceFog->GetEndDistance();
	const Vector3 pathOffset = { 0.0f, gameplayCavePathOffsetY_, 0.0f };
	const float caveStartDistance = caveRailPath_->GetTotalEntryLength();
	const float repeatLength = caveRailPath_->GetRepeatLength();
	const int64_t viewerCycle = repeatLength > 0.0f ?
		std::max<int64_t>(0, static_cast<int64_t>(std::floor(
			std::max(0.0f, gameplayCaveViewerDistance_ - caveStartDistance) /
			repeatLength))) : 0;
	const int64_t firstCycle = std::max<int64_t>(0, viewerCycle - 1);
	for (size_t i = 0; i < caveObjects_.size(); ++i)
	{
		const int64_t cycleIndex = firstCycle + static_cast<int64_t>(i);
		const Vector3 cycleTranslation = pathOffset +
			caveRailPath_->GetSettings().repeatOffset * static_cast<float>(cycleIndex);
		const Matrix4x4 sourceWorld = MakeTranslateMatrix(cycleTranslation);
		const Matrix4x4 displayWorld = MakeDisplayMatrix(sourceWorld, gameplayCaveViewerDistance_);
		caveVisibility_[i] = true;
		caveObjects_[i]->Update(&displayWorld, nullptr, false);
	}

	const float forwardCoverage = settings.field.spacing *
		static_cast<float>(settings.field.poolCount);
	EnsureCrystalPlacements(gameplayCaveViewerDistance_ + forwardCoverage);
	const float crystalStartDistance = std::max(
		caveStartDistance + settings.crystal.entranceClearance,
		gameplayCaveViewerDistance_ - settings.field.behindDistance);
	auto placement = std::lower_bound(
		crystalPlacements_.begin(), crystalPlacements_.end(), crystalStartDistance,
		[](const CrystalPlacement& value, float distance)
		{
			return value.distance < distance;
		});
	for (size_t i = 0; i < crystalObjects_.size(); ++i)
	{
		if (placement == crystalPlacements_.end() ||
			placement->distance > gameplayCaveViewerDistance_ + forwardCoverage)
		{
			crystalVisibility_[i] = false;
			continue;
		}

		const RailFrame frame = caveRailPath_->GetFrame(placement->distance);
		const Matrix4x4 sourceWorld = MakeCrystalMatrix(
			frame, pathOffset, placement->angle, settings.crystal.wallRadius,
			placement->scale, placement->roll);
		const Matrix4x4 displayWorld = MakeDisplayMatrix(sourceWorld, gameplayCaveViewerDistance_);
		const Vector3 displayPosition =
			{ displayWorld.m[3][0], displayWorld.m[3][1], displayWorld.m[3][2] };
		crystalVisibility_[i] = !isFogEnabled ||
			Length(displayPosition - referencePosition) <= fogEndDistance + 15.0f;
		crystalObjects_[i]->Update(&displayWorld, nullptr, false);
		++placement;
	}
}

void Terrain::EnsureCrystalPlacements(float requiredDistance)
{
	const CaveCrystalSettings& settings = caveRailPath_->GetSettings().crystal;
	if (!settings.enabled)
	{
		return;
	}

	const float firstDistance = caveRailPath_->GetTotalEntryLength() +
		settings.entranceClearance;
	while (crystalPlacements_.empty() || crystalPlacements_.back().distance < requiredDistance)
	{
		const uint32_t index = static_cast<uint32_t>(crystalPlacements_.size());
		const float intervalRate = HashToUnit(settings.seed, index, 0);
		const float interval = settings.spacingMode == CrystalSpacingMode::Fixed ?
			settings.fixedInterval :
			settings.randomMinInterval +
			(settings.randomMaxInterval - settings.randomMinInterval) * intervalRate;
		CrystalPlacement placement;
		placement.distance = crystalPlacements_.empty() ?
			firstDistance : crystalPlacements_.back().distance + interval;
		placement.angle = HashToUnit(settings.seed, index, 1) * kTwoPi;
		placement.scale = settings.scaleMin +
			(settings.scaleMax - settings.scaleMin) * HashToUnit(settings.seed, index, 2);
		placement.roll = HashToUnit(settings.seed, index, 3) * kTwoPi;
		crystalPlacements_.push_back(placement);
	}
}

Matrix4x4 Terrain::MakeDisplayMatrix(
	const Matrix4x4& sourceWorld, float viewerDistance) const
{
	if (!isGameplayCaveScrolling_ || !isGameplayCaveScrollAnchorInitialized_)
	{
		return sourceWorld;
	}
	const RailFrame viewerFrame = caveRailPath_->GetFrame(viewerDistance);
	const Matrix4x4 viewerWorld = MakeRailFrameMatrix(
		viewerFrame, { 0.0f, gameplayCavePathOffsetY_, 0.0f });
	return sourceWorld * Inverse(viewerWorld) * gameplayCaveScrollAnchor_;
}

void Terrain::SetGameplayCaveScrolling(bool enabled)
{
	if (isGameplayCaveScrolling_ == enabled)
	{
		return;
	}
	isGameplayCaveScrolling_ = enabled;
	isGameplayCaveScrollAnchorInitialized_ = false;
}

void Terrain::SetGameplayCaveEntryMode(bool enabled)
{
	if (mode_ != Mode::GameplayCave || isGameplayCaveEntryMode_ == enabled)
	{
		return;
	}

	isGameplayCaveEntryMode_ = enabled;
	if (caveRailPath_)
	{
		return;
	}
	if (!enabled)
	{
		return;
	}

	// 地表より上へ洞窟区画を出さず、開口部から下へ隙間なく連結する。
	for (size_t i = 0; i < caveTransforms_.size(); ++i)
	{
		caveTransforms_[i].translate =
			{ 0.0f, -kCaveSegmentLength_ * (static_cast<float>(i) + 0.5f), 0.0f };
	}
}

void Terrain::Draw()
{
	if (isDraw_)
	{
		if (mode_ == Mode::TitleCave || mode_ == Mode::GameplayCave)
		{
			for (size_t i = 0; i < caveObjects_.size(); ++i)
			{
				if (mode_ == Mode::TitleCave || caveVisibility_[i])
				{
					caveObjects_[i]->Draw();
				}
			}
			if (mode_ == Mode::GameplayCave && caveRailPath_)
			{
				for (size_t i = 0; i < crystalObjects_.size(); ++i)
				{
					if (crystalVisibility_[i])
					{
						crystalObjects_[i]->Draw();
					}
				}
			}
		}
		else
		{
			object3d_->Draw();
		}
	}
}

void Terrain::Finalize()
{
	caveObjects_.clear();
	caveTransforms_.clear();
	caveVisibility_.clear();
	crystalObjects_.clear();
	crystalVisibility_.clear();
	crystalPlacements_.clear();
	object3d_.reset();
}
