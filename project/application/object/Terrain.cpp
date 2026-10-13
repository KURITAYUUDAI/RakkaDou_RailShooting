#include "Terrain.h"
#include "DistanceFogManager.h"
#include "ModelManager.h"
#include "ImGuiManager.h"

void Terrain::Initialize(Mode mode)
{
	mode_ = mode;
	isGameplayCaveScrolling_ = false;

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
		auto* distanceFog = DistanceFogManager::GetInstance();
		const bool isFogEnabled = distanceFog->IsEnabled();
		const float fogEndDistance = distanceFog->GetEndDistance();

		for (size_t i = 0; i < caveObjects_.size(); ++i)
		{
			if (isGameplayCaveScrolling_)
			{
				caveTransforms_[i].translate.y += caveSpeed_ * deltaTime;
			}

			while (caveTransforms_[i].translate.y - referencePosition.y > kCaveRecycleHalfDistance_)
			{
				caveTransforms_[i].translate.y -= kCaveRecycleDistance_;
			}
			while (caveTransforms_[i].translate.y - referencePosition.y < -kCaveRecycleHalfDistance_)
			{
				caveTransforms_[i].translate.y += kCaveRecycleDistance_;
			}

			const Vector3 difference = caveTransforms_[i].translate - referencePosition;
			caveVisibility_[i] = !isFogEnabled ||
				Length(difference) - kOutFieldBoundingRadius_ <= fogEndDistance;

			caveObjects_[i]->SetTransform(caveTransforms_[i]);
			caveObjects_[i]->Update();
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
	object3d_.reset();
}
