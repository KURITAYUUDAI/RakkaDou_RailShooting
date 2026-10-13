#include "Terrain.h"
#include "ModelManager.h"
#include "ImGuiManager.h"

void Terrain::Initialize(Mode mode)
{
	mode_ = mode;

	if (mode_ == Mode::TitleCave)
	{
		ModelManager::GetInstance()->LoadModel("Field", "OutField.obj");
		caveObjects_.reserve(kCaveObjectCount_);
		caveTransforms_.reserve(kCaveObjectCount_);

		for (size_t i = 0; i < kCaveObjectCount_; ++i)
		{
			auto object = std::make_unique<Object3d>();
			object->Initialize();
			object->SetModel("OutField.obj");
			object->SetEnableLighting(false);
			object->SetEnvironmentCoefficient(0.0f);

			EulerTransform transform{};
			transform.scale = { 1.0f, 1.0f, 1.0f };
			transform.rotate = { 0.0f, 0.0f, 0.0f };
			transform.translate =
				{ 0.0f, -kCaveSegmentLength_ * static_cast<float>(i), 0.0f };

			caveObjects_.push_back(std::move(object));
			caveTransforms_.push_back(transform);
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
	if (mode_ == Mode::TitleCave)
	{
		ImGui::DragFloat("CaveSpeed", &caveSpeed_, 0.1f);
		for (size_t i = 0; i < caveTransforms_.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::DragFloat3("CavePosition", &caveTransforms_[i].translate.x, 0.1f);
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
		if (mode_ == Mode::TitleCave)
		{
			for (const auto& object : caveObjects_)
			{
				object->Draw();
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
	object3d_.reset();
}
